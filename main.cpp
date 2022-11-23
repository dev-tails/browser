#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>

using namespace emscripten;
#else
#include <curl/curl.h>
#endif

#include <iostream>
#include <unordered_map>
#include <regex>
#include <string>
#include <vector>

using namespace std;

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class Node
{
public:
  Node(string tag, string innerText, string href)
  {
    this->tag = tag;
    this->innerText = innerText;
    this->href = href;
  }

  string tag;
  string innerText;
  string href;

  int x;
  int y;
  int w;
  int h;
};

bool done = false;

unordered_map<string, TTF_Font *> tag_font_map;
unordered_map<string, int> tag_y_margin_map;

int line_index = 0;

SDL_Window *window;
SDL_Renderer *renderer;

string url_address = "https://devtails.xyz";
string html_content;
vector<Node *> nodes;

int y_offset = 0;

void clear();
void render();

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

class HTMLElement
{
public:
  string tagName;
  vector<HTMLElement *> children;
  HTMLElement *parentElement;
  string textContent;
  string href;
};

enum State
{
  STATE_INIT,
  STATE_START_TAG,
  STATE_READING_TAG,
  STATE_READ_ATTRIBUTE_KEY,
  STATE_READ_ATTRIBUTE_VALUE,
  STATE_END_TAG,
  STATE_BEGIN_CLOSING_TAG
};

bool isWhitespace(char c)
{
  return c == ' ';
}

HTMLElement *HTMLParser(string input)
{
  HTMLElement *root = new HTMLElement();

  State state = STATE_INIT;
  HTMLElement *lastParent = root;
  string tagName = "";
  string attributeName = "";
  string attributeValue = "";
  string href = "";

  for (auto c : input) {
    if (c == '<') {
      state = STATE_START_TAG;
    } else if (state == STATE_START_TAG) {
      if (c == '/') {
        state = STATE_BEGIN_CLOSING_TAG;
      } else if (!isWhitespace(c)) {
        state = STATE_READING_TAG;
        tagName = c;
      }
    } else if (state == STATE_READING_TAG) {
      if (isWhitespace(c)) {
        state = STATE_READ_ATTRIBUTE_KEY;
      } else if(c == '>') {
        state = STATE_END_TAG;

        auto parent = new HTMLElement(); 
        parent->tagName = tagName;
        parent->parentElement = lastParent;
        parent->href = href;

        lastParent->children.push_back(parent);
        lastParent = parent;
      } else {
        tagName += c;
      }
    } else if(state == STATE_READ_ATTRIBUTE_KEY) {
      if (c == '>') {
        state = STATE_END_TAG;

        auto parent = new HTMLElement(); 
        parent->tagName = tagName;
        parent->parentElement = lastParent;
        parent->href = href;

        lastParent->children.push_back(parent);
        lastParent = parent;
      } else if (c == '"') {
        state = STATE_READ_ATTRIBUTE_VALUE;
      } else if (!isWhitespace(c) && c != '=') {
        attributeName += c;
      }
    } else if (state == STATE_READ_ATTRIBUTE_VALUE) {
      if (c == '"') {
        if (attributeName == "href") {
          href = attributeValue;
        }
        attributeName = "";
        attributeValue = "";
        state = STATE_READ_ATTRIBUTE_KEY;
      } else if (!isWhitespace(c)) {
        attributeValue += c;
      }
    } else if (state == STATE_END_TAG) {
      lastParent->textContent += c;
    } else if (state == STATE_BEGIN_CLOSING_TAG) {
      if (c == '>') {
        lastParent = lastParent->parentElement;
      }
    }
  }

  return root;
}

void recurse_html_elements(HTMLElement *el)
{
  nodes.push_back(new Node(el->tagName, el->textContent, el->href));

  for (auto child : el->children)
  {
    recurse_html_elements(child);
  }
}

void parse_nodes_from_content(string content)
{
  nodes.clear();

  auto root = HTMLParser(content);
  recurse_html_elements(root);
}

void fetch_page(const string &url)
{
#if __EMSCRIPTEN__
  val text = val::take_ownership(fetch_html(url.c_str()));

  std::string html_content = text.as<std::string>();

  parse_nodes_from_content(html_content);
#else
  CURL *curl_handle = curl_easy_init();

  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  struct memory chunk = {0};

  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  CURLcode res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK)
  {
    fprintf(stderr, "error: %s\n", curl_easy_strerror(res));
  }
  curl_easy_cleanup(curl_handle);

  parse_nodes_from_content(string(chunk.response));

#endif
}

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
    case SDLK_BACKSPACE:
      url_address.pop_back();
      render();
      break;
    case SDLK_RETURN:
      fetch_page(url_address);
      render();
      break;
    }
  }
  else if (event->type == SDL_MOUSEBUTTONDOWN)
  {
    int x, y;
    SDL_GetMouseState(&x, &y);

    x *= 2;
    y *= 2;

    for (auto node : nodes) {
      if (node->tag != "a") {
        continue;
      }
      if (node->x <= x && x <= node->x + node->w) {
        if (node->y <= y && y <= node->y + node->h) {
          url_address = "https://devtails.xyz" + node->href;
          y_offset = 0;
          fetch_page(url_address);
          render();
          break;
        }
      }
    }
  }
  else if (event->type == SDL_TEXTINPUT)
  {
    url_address += event->text.text;
    render();
  }

  return 0;
}

void handle_events()
{
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0)
  {
    event_filter(NULL, &e);
  }
}

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(EM_VAL, fetch_html, (const char *url), {
  url = UTF8ToString(url);
  let response = await fetch(url);
  let text = await response.text();

  return Emval.toHandle(text);
});
#endif

void loop(void)
{
  handle_events();
}

int main(int argc, char *argv[])
{
  string url = "https://devtails.xyz/@adam/building-a-web-browser-with-sdl-in-c++";

  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  tag_font_map.insert(pair<string, TTF_Font *>("h1", TTF_OpenFont("./assets/arial-bold.ttf", 48)));
  tag_font_map.insert(pair<string, TTF_Font *>("h2", TTF_OpenFont("./assets/arial-bold.ttf", 44)));
  tag_font_map.insert(pair<string, TTF_Font *>("h3", TTF_OpenFont("./assets/arial-bold.ttf", 40)));
  tag_font_map.insert(pair<string, TTF_Font *>("h4", TTF_OpenFont("./assets/arial-bold.ttf", 36)));
  tag_font_map.insert(pair<string, TTF_Font *>("h5", TTF_OpenFont("./assets/arial-bold.ttf", 32)));
  tag_font_map.insert(pair<string, TTF_Font *>("h6", TTF_OpenFont("./assets/arial-bold.ttf", 28)));
  tag_font_map.insert(pair<string, TTF_Font *>("p", TTF_OpenFont("./assets/arial.ttf", 24)));
  tag_font_map.insert(pair<string, TTF_Font *>("a", TTF_OpenFont("./assets/arial.ttf", 24)));

  tag_y_margin_map.insert(pair<string, int>("h1", 24));
  tag_y_margin_map.insert(pair<string, int>("h2", 24));
  tag_y_margin_map.insert(pair<string, int>("h3", 20));
  tag_y_margin_map.insert(pair<string, int>("h4", 20));
  tag_y_margin_map.insert(pair<string, int>("h5", 16));
  tag_y_margin_map.insert(pair<string, int>("h6", 16));
  tag_y_margin_map.insert(pair<string, int>("p", 16));

  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);

  window = SDL_CreateWindow("browser",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            DM.w,
                            DM.h,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

  renderer = SDL_CreateRenderer(window,
                                -1,
                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

#if __EMSCRIPTEN__
  render();
  emscripten_set_main_loop(loop, 0, 1);
#else
  SDL_SetEventFilter(event_filter, NULL);

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
#endif

  return 0;
}

void clear()
{
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
}

void render_text(TTF_Font *font, const char *text, SDL_Color &color, int y)
{
  SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, text, color, 1600);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  int texW = 0;
  int texH = 0;
  SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
  SDL_Rect dstrect = {16, y_offset + y, texW, texH};

  SDL_RenderCopy(renderer, texture, NULL, &dstrect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void render_address_bar()
{
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = 1600;
  rect.h = 48;
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(renderer, &rect);

  auto it = tag_font_map.find("p");
  SDL_Color color = {0, 0, 0};
  render_text(it->second, url_address.c_str(), color, 0);
}

void render()
{
  clear();

  SDL_Color color = {255, 255, 255};

  int y = 48 + 16;

  render_address_bar();

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

    node->x = 16;
    node->y = y_offset + y;
    node->w = texW;
    node->h = texH;

    SDL_Rect dstrect = {node->x, node->y, node->w, node->h};
    int y_margin = 0;
    auto it2 = tag_y_margin_map.find(node->tag);
    if (it2 != tag_y_margin_map.end())
    {
      y_margin = it2->second;
    }

    y += texH + y_margin;

    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
  }

  SDL_RenderPresent(renderer);
}