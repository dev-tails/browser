#include <iostream>
#include <unordered_map>
#include <regex>
#include <string>
#include <vector>
#include <curl/curl.h>

using namespace std;

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class Node
{
public:
  Node(string tag, string innerText)
  {
    this->tag = tag;
    this->innerText = innerText;
  }

  string tag;
  string innerText;
};

bool done = false;

unordered_map<string, TTF_Font *> tag_font_map;

int line_index = 0;

SDL_Window *window;
SDL_Renderer *renderer;

string html_content;
vector<Node *> nodes;

int y_offset = 0;

void clear();
void render();

static int SDLCALL event_filter(void *userdata, SDL_Event *event)
{
  if (event->type == SDL_QUIT)
  {
    return 1;
  }
  else if (event->type == SDL_KEYDOWN)
  {
    switch (event->key.keysym.sym)
    {
    case SDLK_UP:
      y_offset += 100;
      render();
      break;
    case SDLK_DOWN:
      y_offset -= 100;
      render();
      break;
    }
  }

  return 0;
}

struct memory
{
  char *response;
  size_t size;
};

static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;

  char *ptr = (char *)realloc(mem->response, mem->size + realsize + 1);
  if (ptr == NULL)
    return 0; /* out of memory! */

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

int main(int argc, char *argv[])
{
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  tag_font_map.insert(pair<string, TTF_Font *>("h1", TTF_OpenFont("arial.ttf", 48)));
  tag_font_map.insert(pair<string, TTF_Font *>("h2", TTF_OpenFont("arial.ttf", 44)));
  tag_font_map.insert(pair<string, TTF_Font *>("h3", TTF_OpenFont("arial.ttf", 40)));
  tag_font_map.insert(pair<string, TTF_Font *>("h4", TTF_OpenFont("arial.ttf", 36)));
  tag_font_map.insert(pair<string, TTF_Font *>("h5", TTF_OpenFont("arial.ttf", 32)));
  tag_font_map.insert(pair<string, TTF_Font *>("h6", TTF_OpenFont("arial.ttf", 28)));
  tag_font_map.insert(pair<string, TTF_Font *>("p", TTF_OpenFont("arial.ttf", 24)));

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

  CURL *curl_handle = curl_easy_init();

  curl_easy_setopt(curl_handle, CURLOPT_URL, "https://devtails.xyz/breadth-first-search-a-walk-in-the-park");
  struct memory chunk = {0};

  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  CURLcode res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK)
  {
    fprintf(stderr, "error: %s\n", curl_easy_strerror(res));
  }
  curl_easy_cleanup(curl_handle);

  string response_text = string(chunk.response);

  std::regex tag_regex("<(h\\d|p).*>(.*)</(h\\d|p)>");
  auto words_begin =
      std::sregex_iterator(response_text.begin(), response_text.end(), tag_regex);
  auto words_end = std::sregex_iterator();

  for (std::sregex_iterator i = words_begin; i != words_end; ++i)
  {
    std::smatch match = *i;
    string tag = match[1].str();
    string text = match[2].str();
    nodes.push_back(new Node(tag, text));
  }

  render();

  SDL_Event event;
  SDL_WaitEvent(&event);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  for (auto it : tag_font_map)
  {
    TTF_CloseFont(it.second);
  }
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

  int y = 0;
  for (auto node : nodes)
  {
    auto it = tag_font_map.find(node->tag);
    if (it == tag_font_map.end())
    {
      continue;
    }

    TTF_Font *font = it->second;

    SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, node->innerText.c_str(), color, 1600);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
    SDL_Rect dstrect = {0, y_offset + y, texW, texH};
    y += texH;

    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
  }

  SDL_RenderPresent(renderer);
}