#include "SDL3/SDL_init.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_render.h"
#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#define screen_width  1280
#define screen_height 720

void draw_grid(SDL_Renderer *renderer, float scale);

typedef struct AppData {
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  float scale;
} AppData;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

  AppData *appdata = malloc(sizeof(AppData));

  if (appdata == NULL) {
    SDL_Log("Failed to allocate memory for AppData");
    return SDL_APP_FAILURE;
  }

  *appdata       = (AppData){.window = NULL, .renderer = NULL, .font = NULL};
  *appstate      = appdata;
  appdata->scale = 1.0f;

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
  int img_flags = IMG_INIT_PNG; // IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF
  if (!(IMG_Init(img_flags) & img_flags)) {
    SDL_Log("Image_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  // TODO add font init
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  AppData *appdata = (AppData *)appstate;
  switch (event->type) {
    case (SDL_EVENT_QUIT):
      return SDL_APP_SUCCESS;
      break;

    case (SDL_EVENT_KEY_DOWN):

      switch (event->key.key) {
        case (SDLK_ESCAPE):
          return SDL_APP_SUCCESS;
          break;
        default:
          break;
      }
    case (SDL_EVENT_MOUSE_WHEEL): {
      float scale = event->wheel.y;
      appdata->scale += scale;
      default:
        break;
    }
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  AppData *appdata       = (AppData *)appstate;
  SDL_Renderer *renderer = appdata->renderer;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  draw_grid(renderer, appdata->scale);

  SDL_RenderPresent(renderer);

  SDL_Delay(1000 / 60); // 60 FPS

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  AppData *appdata = (AppData *)appstate;
  SDL_DestroyRenderer(appdata->renderer);
  SDL_DestroyWindow(appdata->window);
  TTF_Quit();
  IMG_Quit();
}

// =================================================================================

void draw_grid(SDL_Renderer *renderer, float scale) {
  int cellSize    = 25;
  int bigCellSize = 10 * cellSize;

  for (int x = 0; x < screen_width; x += bigCellSize) {
    for (int y = 0; y < screen_height; y += bigCellSize) {
      // Рисуем большой квадрат
      SDL_FRect rect = {x, y, bigCellSize * scale, bigCellSize * scale};
      SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Серый цвет для больших квадратов
      SDL_RenderRect(renderer, &rect);

      // Рисуем маленькие квадраты внутри большого квадрата
      for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 100; ++j) {
          SDL_FRect smallRect
            = {x + i * cellSize, y + j * cellSize, cellSize * scale, cellSize * scale};
          SDL_SetRenderDrawColor(
            renderer, 220, 220, 220, 125); // Светло-серый цвет для маленьких квадратов
          SDL_RenderRect(renderer, &smallRect);
        }
      }
    }
  }
}
