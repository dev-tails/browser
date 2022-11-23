#include <string>
#include <vector>
#include <cassert>
#include <iostream>

using namespace std;

class HTMLElement
{
public:
  string tagName;
  vector<struct HTMLElement *> children;
  struct HTMLElement *parentElement;
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

int main()
{
  HTMLElement *el = HTMLParser("<a class=\"ugh\" href=\"https://devtails.xyz\">Dev/Tails</a>");

  assert(el->children.size() == 1);
  assert(el->children[0]->tagName == "a");
  assert(el->children[0]->href == "https://devtails.xyz");

  return 0;
}