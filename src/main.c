#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#define screen_width  1280
#define screen_height 720

typedef struct AppData {
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  float scale;
  SDL_Texture *gameboard;
  float global_x, global_y;
} AppData;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

  AppData *appdata = malloc(sizeof(AppData));

  if (appdata == NULL) {
    SDL_Log("Failed to allocate memory for AppData");
    return SDL_APP_FAILURE;
  }

  *appdata = (AppData){.window = NULL, .renderer = NULL, .font = NULL};
  *appstate = appdata;

  appdata->scale = 0.5f;
  appdata->global_x = 0;
  appdata->global_y = 0;

  if (!(SDL_Init(SDL_INIT_VIDEO))) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  } else if (!(appdata->window = SDL_CreateWindow("TABLETOP", screen_width, screen_height, 0))) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!(appdata->renderer = SDL_CreateRenderer(appdata->window, 0))) {
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

  appdata->gameboard = SDL_CreateTextureFromSurface(appdata->renderer, img_surface);
  if (!appdata->gameboard) {
    SDL_Log("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_DestroySurface(img_surface);

  // TODO add font init
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  AppData *appdata = (AppData *)appstate;
  float factor = 0.01;
  float future_scale;
  float max_zoom = 0.2; // 20% of image is maximal zoom

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
      future_scale = appdata->scale + event->wheel.y * factor;

      if ((future_scale * appdata->gameboard->h) >= screen_height
          && (future_scale * appdata->gameboard->h) * max_zoom <= screen_height) {
        appdata->scale = future_scale;

        break;
      }
    case (SDL_EVENT_MOUSE_MOTION):
      if ((event->motion.xrel != 0 || event->motion.yrel != 0)
          && (event->motion.state == SDL_BUTTON_LEFT)) {
        appdata->global_x += event->motion.xrel * appdata->scale * 5;
        appdata->global_y += event->motion.yrel * appdata->scale * 5;

        break;
      }
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  AppData *appdata = (AppData *)appstate;
  SDL_Renderer *renderer = appdata->renderer;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  // ======== Draw here ==============
  float scale = appdata->scale;
  float texture_width = appdata->gameboard->w;
  float texture_height = appdata->gameboard->h;

  SDL_FRect img_rect = {.x = appdata->global_x + (screen_width / 2.f),
                        .y = appdata->global_y + (screen_height / 2.f),
                        .w = texture_width * scale,
                        .h = texture_height * scale};

  SDL_RenderTexture(renderer, appdata->gameboard, NULL, &img_rect);

  SDL_RenderTexture(renderer, appdata->gameboard, NULL, &img_rect);
  // ==================================

  SDL_RenderPresent(renderer);
  SDL_Delay(1000 / 60); // 60 FPS

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  AppData *appdata = (AppData *)appstate;
  SDL_DestroyRenderer(appdata->renderer);
  SDL_DestroyWindow(appdata->window);
  SDL_DestroyTexture(appdata->gameboard);
  TTF_Quit();
  IMG_Quit();
  free(appdata);
}

// =================================================================================
