CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CORE = CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp Timer.cpp Joypad.cpp
HEADERS = $(wildcard *.h)

.PHONY: all headless test
all: emulator

emulator: main.cpp Display.cpp $(CORE) $(HEADERS)
	@pkg-config --exists sdl3 || { echo "Install SDL3 development files and pkg-config (see README.md)."; exit 1; }
	$(CXX) $(CXXFLAGS) -DGB_WITH_SDL main.cpp Display.cpp $(CORE) $$(pkg-config --cflags --libs sdl3) -o $@

headless: emulator-headless
emulator-headless: main.cpp Display.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp Display.cpp $(CORE) -o $@

test: test-graphics test-cpu-control
	./test-graphics
	./test-cpu-control

test-graphics: test_graphics.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_graphics.cpp $(CORE) -o $@

test-cpu-control: test_cpu_control.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_cpu_control.cpp $(CORE) -o $@
