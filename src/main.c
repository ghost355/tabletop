#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#define screen_width  (1201 * 1.5)
#define screen_height (709 * 1.5)

#define border_move_zone 10
#define motion_speed     1000
#define target_fps       60.0
#define max_frame_time   (1.0 / target_fps)

typedef enum { N, NE, E, SE, S, SW, W, NW, NoDirection } PanDirection;

typedef struct AppData {

  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  SDL_Texture *gameboard;

  float scale;
  float offset_x, offset_y;
  float motion_factor;
  int cursor_x, cursor_y;
  float wheel_y;
  const bool *key_states;

  double dt;
  Uint64 last_time;

  bool in_window;
} AppData;

// ============ Function declaration =============

void zoom_world(AppData *data);
void border_out_control(AppData *data);
PanDirection handle_pan_input(AppData *data);
void pan_world(AppData *data);

// ===============================================

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

  AppData *data = malloc(sizeof(AppData));
  if (data == NULL) {
    SDL_Log("Failed to allocate memory for AppData");
    return SDL_APP_FAILURE;
  }

  *data = (AppData){.window = NULL, .renderer = NULL, .font = NULL};

  data->scale         = screen_height / 4252.0f;
  data->offset_x      = 0;
  data->offset_y      = 0;
  data->motion_factor = 3.0f;
  data->cursor_x      = 0;
  data->cursor_y      = 0;
  data->dt            = 0.0;
  data->last_time     = SDL_GetPerformanceCounter();
  data->in_window     = true;
  data->wheel_y       = 0;
  data->key_states    = NULL;

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
  char *img_path           = "assets/tabletop.png";
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
  *appstate = data;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  AppData *data = (AppData *)appstate;
  // TODO: Here will be handle_input(AppData *date)**

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
      data->wheel_y = event->wheel.y;
      break;

    case SDL_EVENT_MOUSE_MOTION:
      data->cursor_x = event->motion.x;
      data->cursor_y = event->motion.y;
      break;

    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
      data->in_window = false;
      break;
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
      data->in_window = true;
      break;

    default:
      break;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

  // TODO: Here will be update(AppData *date)**

  AppData *data          = (AppData *)appstate;
  SDL_Renderer *renderer = data->renderer;

  // =========== Delta Time ===========
  Uint64 currentTime  = SDL_GetPerformanceCounter();
  Uint64 elapsedTicks = currentTime - data->last_time;
  data->last_time     = currentTime;

  double elapsedSeconds = (double)elapsedTicks / SDL_GetPerformanceFrequency();
  data->dt += elapsedSeconds;

  data->key_states = SDL_GetKeyboardState(NULL);

  while (data->dt >= max_frame_time) {

    // Update game logic here using deltaTime
    pan_world(data);
    zoom_world(data);
    border_out_control(data);

    data->dt -= max_frame_time;
  }

  // ===== RENDER SECTION ============

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  // ======== Draw here ==============
  float scale          = data->scale;
  float texture_width  = data->gameboard->w;
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

void zoom_world(AppData *data) {
  if (data->wheel_y != 0) {
    float zoom_factor = (data->wheel_y > 0) ? 1.1f : 0.9f;
    float new_scale   = data->scale * zoom_factor;

    if (new_scale <= (float)screen_height / data->gameboard->h) {
      new_scale = (float)screen_height / data->gameboard->h;
    }
    if (new_scale > 1.6f) {
      new_scale = 1.6f;
    }
    float relative_x = (data->cursor_x - data->offset_x) / data->scale;
    float relative_y = (data->cursor_y - data->offset_y) / data->scale;

    data->offset_x = data->cursor_x - relative_x * new_scale;
    data->offset_y = data->cursor_y - relative_y * new_scale;

    data->scale = new_scale;
  }
  data->wheel_y = 0;
}

void border_out_control(AppData *data) {
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

PanDirection handle_pan_input(AppData *data) {

  PanDirection pan_direction = NoDirection;

  // NOTE: pan with cursor on screen edge
  if (data->in_window) {
    if (data->cursor_x <= border_move_zone) {
      pan_direction = W;
    }
    if (data->cursor_x >= screen_width - border_move_zone) {
      pan_direction = E;
    }
    if (data->cursor_y <= border_move_zone) {
      pan_direction = N;
    }
    if (data->cursor_y >= screen_height - border_move_zone) {
      pan_direction = S;
    }
    if (data->cursor_x <= border_move_zone && data->cursor_y <= border_move_zone) {
      pan_direction = NW;
    }
    if (data->cursor_x >= screen_width - border_move_zone && data->cursor_y <= border_move_zone) {
      pan_direction = NE;
    }
    if (data->cursor_x <= border_move_zone && data->cursor_y >= screen_height - border_move_zone) {
      pan_direction = SW;
    }
    if (data->cursor_x >= screen_width - border_move_zone
        && data->cursor_y >= screen_height - border_move_zone) {
      pan_direction = SE;
    }
  }
  // NOTE: pan with keys WASD
  if (data->key_states) {
    if (data->key_states[SDL_SCANCODE_W]) {
      pan_direction = N;
    }
    if (data->key_states[SDL_SCANCODE_S]) {
      pan_direction = S;
    }
    if (data->key_states[SDL_SCANCODE_A]) {
      pan_direction = W;
    }
    if (data->key_states[SDL_SCANCODE_D]) {
      pan_direction = E;
    }
    if (data->key_states[SDL_SCANCODE_S] && data->key_states[SDL_SCANCODE_A]) {
      pan_direction = SW;
    }
    if (data->key_states[SDL_SCANCODE_W] && data->key_states[SDL_SCANCODE_A]) {
      pan_direction = NW;
    }
    if (data->key_states[SDL_SCANCODE_S] && data->key_states[SDL_SCANCODE_D]) {
      pan_direction = SE;
    }
    if (data->key_states[SDL_SCANCODE_W] && data->key_states[SDL_SCANCODE_D]) {
      pan_direction = NE;
    }
  }
  return pan_direction;
}

void pan_world(AppData *data) {

  PanDirection pan_direction = handle_pan_input(data);

  switch (pan_direction) {
    case E:
      data->offset_x -= motion_speed * data->scale * data->dt;
      break;
    case W:
      data->offset_x += motion_speed * data->scale * data->dt;
      break;
    case S:
      data->offset_y -= motion_speed * data->scale * data->dt;
      break;
    case N:
      data->offset_y += motion_speed * data->scale * data->dt;
      break;
    case SW:
      data->offset_y -= motion_speed * data->scale * data->dt;
      data->offset_x += motion_speed * data->scale * data->dt;
      break;
    case NW:
      data->offset_x += motion_speed * data->scale * data->dt;
      data->offset_y += motion_speed * data->scale * data->dt;
      break;
    case NE:
      data->offset_y += motion_speed * data->scale * data->dt;
      data->offset_x -= motion_speed * data->scale * data->dt;
      break;
    case SE:
      data->offset_y -= motion_speed * data->scale * data->dt;
      data->offset_x -= motion_speed * data->scale * data->dt;
      break;

    default:
      break;
  }
}
