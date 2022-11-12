#include <stdbool.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

bool done = false;
const int font_size = 64;
int line_index = 0;

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font = NULL;

void clear();
void render();

static int SDLCALL event_filter(void *userdata, SDL_Event *event)
{
  if (event->type == SDL_QUIT)
  {
    return 1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  SDL_Init(SDL_INIT_VIDEO);

  TTF_Init();
  font = TTF_OpenFont("arial.ttf", font_size);
  if (!font)
  {
    return 1;
  }

  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);

  SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );

  window = SDL_CreateWindow("browser",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            DM.w,
                            DM.h,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

  renderer = SDL_CreateRenderer(window,
                                -1,
                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  clear();

  SDL_SetEventFilter(event_filter, NULL);

  render();

  SDL_Event event;
  SDL_WaitEvent(&event);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  TTF_CloseFont(font);
  TTF_Quit();
  SDL_Quit();

  return 0;
}

void clear()
{
  SDL_RenderClear(renderer);
}

void render()
{
  clear();

  SDL_Color color = {255, 255, 255};
  SDL_Surface *surface = TTF_RenderText_Blended(font, "Page Title", color);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  int texW = 0;
  int texH = 0;
  SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
  SDL_Rect dstrect = {0, 0, texW, texH};

  SDL_RenderCopy(renderer, texture, NULL, &dstrect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
  SDL_RenderPresent(renderer);
}