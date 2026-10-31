# Mini Notepad++ derleme ve kurulum
#   make                 build/mini-notepad derlenir
#   make install         ~/.local/bin altına kurar (sudo gerekmez)
#   make uninstall       kurulumu kaldırır
#   make clean           derleme çıktılarını siler
# Başka bir yere kurmak için: make install PREFIX=/usr/local

CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra
PREFIX   ?= $(HOME)/.local

BIN     := build/mini-notepad
SOURCES := $(wildcard *.cpp)
HEADERS := $(wildcard *.h)

all: $(BIN)

$(BIN): $(SOURCES) $(HEADERS)
	@mkdir -p build
	$(CXX) -std=c++17 $(CXXFLAGS) $(SOURCES) -o $@

install: $(BIN)
	@mkdir -p $(PREFIX)/bin
	install -m 755 $(BIN) $(PREFIX)/bin/mini-notepad
	@echo "Kuruldu: $(PREFIX)/bin/mini-notepad"

uninstall:
	rm -f $(PREFIX)/bin/mini-notepad

clean:
	rm -rf build

.PHONY: all install uninstall clean
