#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#define screen_width  1201
#define screen_height 709

typedef struct AppData {
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  float scale;
  SDL_Texture *gameboard;
  float offset_x, offset_y;
  float motion_factor;
} AppData;

void zoom_to_cursor(AppData *data, float zoom_factor, int cursor_x, int cursor_y);

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
  if (!(TTF_Init())) {
    SDL_Log("TTF_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  int img_flags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF;
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
    case (SDL_EVENT_QUIT):
      return SDL_APP_SUCCESS;
      break;

    case (SDL_EVENT_KEY_DOWN):
      switch (event->key.key) {
        case (SDLK_ESCAPE):
        case (SDLK_Q):
          return SDL_APP_SUCCESS;
          break;
        default:
          break;
      }
    case (SDL_EVENT_MOUSE_WHEEL):
      if (event->wheel.y > 0) {
        zoom_to_cursor(data, 1.1f, event->wheel.mouse_x, event->wheel.mouse_y);
      } else if (event->wheel.y < 0) {
        zoom_to_cursor(data, 0.9f, event->wheel.mouse_x, event->wheel.mouse_y);
      }
      break;

    case (SDL_EVENT_MOUSE_MOTION):

      if (event->motion.state == SDL_BUTTON_LEFT) {
        data->offset_x += event->motion.xrel * data->scale * data->motion_factor;
        data->offset_y += event->motion.yrel * data->scale * data->motion_factor;
      }

      break;
    default:
      break;
  }
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

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  AppData *data = (AppData *)appstate;
  SDL_Renderer *renderer = data->renderer;
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
  SDL_Delay(1000 / 60); // 60 FPS

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

// =================================================================================

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
