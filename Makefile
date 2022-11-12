CC = c++
CFLAGS = -std=c++17 -Wall -I /opt/homebrew/include -lcurl
TARGET = browser

LDFLAGS = `pkg-config --libs --static SDL2 SDL2_ttf`

browser: main.cpp
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

web:
	emcc main.cpp -s USE_SDL=2 -s USE_SDL_TTF=2 --preload-file assets --shell-file emscripten/template/index.html -o public/index.html

# https://stackoverflow.com/questions/39892692/how-to-implement-make-install-in-a-makefile
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

install: main.c
	install browser $(DESTDIR)$(PREFIX)/bin/

.PHONY: clean
clean:
	$(RM) $(TARGET)