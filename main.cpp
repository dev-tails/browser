#include <iostream>
#include <regex>
#include <string>
#include <vector>

using namespace std;

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

bool done = false;
const int font_size_h1 = 48;
const int font_size_p = 24;
int line_index = 0;

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font_h1 = NULL;
TTF_Font *font_p = NULL;

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
  font_h1 = TTF_OpenFont("arial.ttf", font_size_h1);
  font_p = TTF_OpenFont("arial.ttf", font_size_p);

  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

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

  TTF_CloseFont(font_h1);
  TTF_Quit();
  SDL_Quit();

  return 0;
}

void clear()
{
  SDL_RenderClear(renderer);
}

typedef struct Node
{
  string tag;
  string text;
} Node;

void render()
{
  clear();

  SDL_RWops *file = SDL_RWFromFile("index.html", "r");
  char buf[1024];
  SDL_RWread(file, buf, sizeof(buf), 1);
  SDL_RWclose(file);

  vector<Node> nodes;

  string tag = buf;

  std::regex tag_regex("<(h\\d|p)>(.*)</(h\\d|p)>");
  auto words_begin =
      std::sregex_iterator(tag.begin(), tag.end(), tag_regex);
  auto words_end = std::sregex_iterator();

  SDL_Color color = {255, 255, 255};

  int y = 0;
  for (std::sregex_iterator i = words_begin; i != words_end; ++i)
  {
    std::smatch match = *i;
    string tag = match[1].str();
    string text = match[2].str();

    TTF_Font *font = tag == "h1" ? font_h1 : font_p;
    int font_size = tag == "h1" ? font_size_h1 : font_size_p;

    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), color);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
    SDL_Rect dstrect = {0, y, texW, texH};

    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    y += font_size;
  }

  SDL_RenderPresent(renderer);
}