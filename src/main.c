#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#define screen_width  (1201 * 1.5)
#define screen_height (709 * 1.5)

#define borderPanZone 20
#define panSpeed      1000
#define targetFPS     60.0
#define maxFrameTime  (1.0 / targetFPS)

#define N_BIT  0x01 // North     00000001
#define NE_BIT 0x02 // NorthEast 00000010
#define E_BIT  0x04 // East      00000100
#define SE_BIT 0x08 // SouthEast 00001000
#define S_BIT  0x10 // South     00010000
#define SW_BIT 0x20 // SouthWest 00100000
#define W_BIT  0x40 // West      01000000
#define NW_BIT 0x80 // NorthWest 10000000

typedef struct AppData {

  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  SDL_Texture *gameboard;

  float scale;
  float offset_x, offset_y;
  int cursor_x, cursor_y;
  float wheel_y;
  const bool *key_states;
  Uint8 pan_direction;

  double dt;
  Uint32 last_time;

  bool in_window;
} AppData;

// ============ Function declaration =============

void zoom_world(AppData *data);
void border_out_control(AppData *data);
void handle_pan_input(AppData *data);
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
  data->cursor_x      = 0;
  data->cursor_y      = 0;
  data->dt            = 0.0;
  data->last_time     = SDL_GetPerformanceCounter();
  data->in_window     = true;
  data->wheel_y       = 0;
  data->key_states    = NULL;
  data->pan_direction = 0;

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
  Uint32 currentTime  = SDL_GetPerformanceCounter();
  Uint32 elapsedTicks = currentTime - data->last_time;
  data->last_time     = currentTime;

  double elapsedSeconds = (double)elapsedTicks / SDL_GetPerformanceFrequency();
  data->dt += elapsedSeconds;

  data->key_states = SDL_GetKeyboardState(NULL);

  while (data->dt >= maxFrameTime) {

    // Update game logic here using deltaTime
    pan_world(data);

    if (data->wheel_y != 0) {
      zoom_world(data);
    }
    border_out_control(data);

    data->dt -= maxFrameTime;
  }

  // ===== RENDER SECTION ============

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  // ======== Draw here ==============

  SDL_FRect img_rect = {.x = data->offset_x,
                        .y = data->offset_y,
                        .w = data->gameboard->w * data->scale,
                        .h = data->gameboard->h * data->scale};

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

void handle_pan_input(AppData *data) {

  data->pan_direction = 0;

  // NOTE: pan with keys WASD
  if (data->key_states) {
    if (data->key_states[SDL_SCANCODE_W]) {
      data->pan_direction |= N_BIT;
    }
    if (data->key_states[SDL_SCANCODE_S]) {
      data->pan_direction |= S_BIT;
    }
    if (data->key_states[SDL_SCANCODE_A]) {
      data->pan_direction |= W_BIT;
    }
    if (data->key_states[SDL_SCANCODE_D]) {
      data->pan_direction |= E_BIT;
    }
  }

  // NOTE: pan with cursor on screen edge
  if (data->in_window) {
    if (data->cursor_x <= borderPanZone) {
      data->pan_direction |= W_BIT;
    }
    if (data->cursor_x >= screen_width - borderPanZone) {
      data->pan_direction |= E_BIT;
    }
    if (data->cursor_y <= borderPanZone) {
      data->pan_direction |= N_BIT;
    }
    if (data->cursor_y >= screen_height - borderPanZone) {
      data->pan_direction |= S_BIT;
    }
  }
}

void pan_world(AppData *data) {
  handle_pan_input(data);
  float speed = panSpeed * data->scale * data->dt;

  if (data->pan_direction & N_BIT) {
    data->offset_y += speed;
  }
  if (data->pan_direction & S_BIT) {
    data->offset_y -= speed;
  }
  if (data->pan_direction & E_BIT) {
    data->offset_x -= speed;
  }
  if (data->pan_direction & W_BIT) {
    data->offset_x += speed;
  }
}

// NOTE: нейросеть советовала оптимизировать рендереинг если не было изменеия положения, то не
// отрисовывать
// NOTE: может быть конфликт панорамирования мира при нажатии кнопок и при одновременном
// положении курсора на краях экрана
