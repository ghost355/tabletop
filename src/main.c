#include "SDL3/SDL_init.h"
#include "SDL3/SDL_scancode.h"
#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#define screen_width     1201
#define screen_height    709
#define border_move_zone 10
#define motion_speed     2000
#define target_fps       60.0
#define max_frame_time   (1.0 / target_fps)

typedef enum { N, NE, E, SE, S, SW, W, NW, NoDirection } PanDirection;

typedef struct AppData {
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  float scale;
  SDL_Texture *gameboard;
  float offset_x, offset_y;
  float motion_factor;
  int cursor_x, cursor_y;
  double dt;
  Uint64 last_time;
  bool in_window;
  PanDirection pan_direction;
} AppData;

void zoom_to_cursor(AppData *data, float zoom_factor, int cursor_x, int cursor_y);
void check_border_out(AppData *data);
bool pan_with_screen_edge_touch(AppData *data);
void pan_world(AppData *data);

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

  AppData *data = malloc(sizeof(AppData));

  if (data == NULL) {
    SDL_Log("Failed to allocate memory for AppData");
    return SDL_APP_FAILURE;
  }

  *data = (AppData){.window = NULL, .renderer = NULL, .font = NULL};
  *appstate = data;

  data->scale = screen_height / 4252.0f;
  data->offset_x = 0;
  data->offset_y = 0;
  data->motion_factor = 3.0f;
  data->cursor_x = 0;
  data->cursor_y = 0;
  data->dt = 0.0;
  data->last_time = SDL_GetPerformanceCounter();
  data->in_window = false;
  data->pan_direction = NoDirection;

  if (!(SDL_Init(SDL_INIT_VIDEO))) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  } else if (!(data->window = SDL_CreateWindow("TABLETOP", screen_width, screen_height, 0))) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!(data->renderer = SDL_CreateRenderer(data->window, 0))) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!(SDL_SetRenderVSync(data->renderer, 1))) {
    SDL_Log("SDL_SetRenderVSync failed: %s", SDL_GetError());
  }

  if (!(TTF_Init())) {
    SDL_Log("TTF_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
  if (!(IMG_Init(img_flags) & img_flags)) {
    SDL_Log("Image_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  // Load image
  char *img_path = "assets/tabletop.png";
  SDL_Surface *img_surface = IMG_Load(img_path);
  if (!img_surface) {
    SDL_Log("IMG_Load failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  data->gameboard = SDL_CreateTextureFromSurface(data->renderer, img_surface);
  if (!data->gameboard) {
    SDL_Log("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_DestroySurface(img_surface);

  // TODO add font init
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {

  AppData *data = (AppData *)appstate;

  switch (event->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
      break;
    case SDL_EVENT_KEY_DOWN:
      switch (event->key.key) {
        case SDLK_ESCAPE:
        case SDLK_Q:
          return SDL_APP_SUCCESS;
          break;
      }
      break;

    case SDL_EVENT_MOUSE_WHEEL:
      if (event->wheel.y > 0) {
        zoom_to_cursor(data, 1.1f, event->wheel.mouse_x, event->wheel.mouse_y);
      } else if (event->wheel.y < 0) {
        zoom_to_cursor(data, 0.9f, event->wheel.mouse_x, event->wheel.mouse_y);
      }
      break;

    case SDL_EVENT_MOUSE_MOTION:
      data->cursor_x = event->motion.x;
      data->cursor_y = event->motion.y;

      if (event->motion.state == SDL_BUTTON_LEFT) {
        data->offset_x += event->motion.xrel * data->scale * data->motion_factor;
        data->offset_y += event->motion.yrel * data->scale * data->motion_factor;
      }
      break;

    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
      data->in_window = !data->in_window;
      break;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  AppData *data = (AppData *)appstate;
  SDL_Renderer *renderer = data->renderer;
  const bool *keyStates = SDL_GetKeyboardState(NULL);
  if (keyStates[SDL_SCANCODE_W]) {
    data->pan_direction = S;
  }
  if (keyStates[SDL_SCANCODE_S]) {
    data->pan_direction = N;
  }
  if (keyStates[SDL_SCANCODE_A]) {
    data->pan_direction = W;
  }
  if (keyStates[SDL_SCANCODE_D]) {
    data->pan_direction = E;
  }
  if (keyStates[SDL_SCANCODE_S] && keyStates[SDL_SCANCODE_A]) {
    data->pan_direction = NW;
  }
  if (keyStates[SDL_SCANCODE_W] && keyStates[SDL_SCANCODE_A]) {
    data->pan_direction = SW;
  }
  if (keyStates[SDL_SCANCODE_S] && keyStates[SDL_SCANCODE_D]) {
    data->pan_direction = NE;
  }
  if (keyStates[SDL_SCANCODE_W] && keyStates[SDL_SCANCODE_D]) {
    data->pan_direction = SE;
  }
  // Delta Time SECTION
  Uint64 currentTime = SDL_GetPerformanceCounter();
  Uint64 elapsedTicks = currentTime - data->last_time;
  data->last_time = currentTime;

  double elapsedSeconds = (double)elapsedTicks / SDL_GetPerformanceFrequency();
  data->dt += elapsedSeconds;

  while (data->dt >= max_frame_time) {
    // Update game logic here using deltaTime

    pan_world(data);
    check_border_out(data);

    data->dt -= max_frame_time;
  }

  // ===== RENDER SECTION ============

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  // ======== Draw here ==============
  float scale = data->scale;
  float texture_width = data->gameboard->w;
  float texture_height = data->gameboard->h;

  SDL_FRect img_rect = {.x = data->offset_x,
                        .y = data->offset_y,
                        .w = texture_width * scale,
                        .h = texture_height * scale};

  SDL_RenderTexture(renderer, data->gameboard, NULL, &img_rect);
  // ==================================

  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  AppData *data = (AppData *)appstate;
  SDL_DestroyRenderer(data->renderer);
  SDL_DestroyWindow(data->window);
  SDL_DestroyTexture(data->gameboard);
  TTF_Quit();
  IMG_Quit();
  free(data);
}

// ============================  Functions  ======================================

void zoom_to_cursor(AppData *data, float zoom_factor, int cursor_x, int cursor_y) {
  float new_scale = data->scale * zoom_factor;
  if (new_scale < 0.165f) {
    new_scale = 0.165f;
  }
  if (new_scale > 1.6f) {
    new_scale = 1.6f;
  }
  float relative_x = (cursor_x - data->offset_x) / data->scale;
  float relative_y = (cursor_y - data->offset_y) / data->scale;

  data->offset_x = cursor_x - relative_x * new_scale;
  data->offset_y = cursor_y - relative_y * new_scale;

  data->scale = new_scale;
}

void check_border_out(AppData *data) {
  if (data->offset_x >= 0) {
    data->offset_x = 0;
  }
  if (data->offset_y >= 0) {
    data->offset_y = 0;
  }
  if (data->offset_x + data->scale * data->gameboard->w <= screen_width) {
    data->offset_x = screen_width - data->scale * data->gameboard->w;
  }
  if (data->offset_y + data->scale * data->gameboard->h <= screen_height) {
    data->offset_y = screen_height - data->scale * data->gameboard->h;
  }
}

bool pan_with_screen_edge_touch(AppData *data) {

  if (data->in_window) {
    if (data->cursor_x <= border_move_zone) {
      data->pan_direction = W;
    }
    if (data->cursor_x >= screen_width - border_move_zone) {
      data->pan_direction = E;
    }
    if (data->cursor_x <= border_move_zone * 0.2) {
      data->pan_direction = W;
    }
    if (data->cursor_x >= screen_width - border_move_zone * 0.2) {
      data->pan_direction = E;
    }

    if (data->cursor_y <= border_move_zone) {
      data->pan_direction = S;
    }
    if (data->cursor_y >= screen_height - border_move_zone) {
      data->pan_direction = N;
    }
    if (data->cursor_y <= border_move_zone * 0.2) {
      data->pan_direction = S;
    }
    if (data->cursor_y >= screen_height - border_move_zone * 0.2) {
      data->pan_direction = N;
      // data->offset_y -= motion_speed * 2 * data->scale * data->dt;
    }
    return true;
  }
  return false;
}

void pan_world(AppData *data) {
  pan_with_screen_edge_touch(data);

  switch (data->pan_direction) {
    case E:
      data->offset_x -= motion_speed * data->scale * data->dt;
      break;
    case W:
      data->offset_x += motion_speed * data->scale * data->dt;
      break;
    case N:
      data->offset_y -= motion_speed * data->scale * data->dt;
      break;
    case S:
      data->offset_y += motion_speed * data->scale * data->dt;
      break;
    case NW:
      data->offset_y -= motion_speed * data->scale * data->dt;
      data->offset_x += motion_speed * data->scale * data->dt;
      break;
    case SW:
      data->offset_x += motion_speed * data->scale * data->dt;
      data->offset_y += motion_speed * data->scale * data->dt;
      break;
    case SE:
      data->offset_y += motion_speed * data->scale * data->dt;
      data->offset_x -= motion_speed * data->scale * data->dt;
      break;
    case NE:
      data->offset_y -= motion_speed * data->scale * data->dt;
      data->offset_x -= motion_speed * data->scale * data->dt;
      break;

    default:
      break;
  }
  data->pan_direction = NoDirection;
}

// TODO: сделать одновременные панорамирование мира - движение по диагонали
