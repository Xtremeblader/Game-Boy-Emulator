CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CORE = APU.cpp CPU.cpp Cartridge.cpp MMU.cpp PPU.cpp Timer.cpp Joypad.cpp
HEADERS = $(wildcard *.h)

.PHONY: all headless test
all: emulator

emulator: main.cpp Display.cpp AudioOutput.cpp $(CORE) $(HEADERS)
	@pkg-config --exists sdl3 || { echo "Install SDL3 development files and pkg-config (see README.md)."; exit 1; }
	$(CXX) $(CXXFLAGS) -DGB_WITH_SDL main.cpp Display.cpp AudioOutput.cpp $(CORE) $$(pkg-config --cflags --libs sdl3) -o $@

headless: emulator-headless
emulator-headless: main.cpp Display.cpp AudioOutput.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp Display.cpp AudioOutput.cpp $(CORE) -o $@

test: test-graphics test-cpu-control test-pokemon-graphics test-input test-timer test-timer-bus test-halt test-battery test-audio
	./test-graphics
	./test-cpu-control
	./test-pokemon-graphics
	./test-input
	./test-timer
	./test-timer-bus
	./test-halt
	./test-battery
	./test-audio

test-graphics: test_graphics.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_graphics.cpp $(CORE) -o $@

test-cpu-control: test_cpu_control.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_cpu_control.cpp $(CORE) -o $@

test-pokemon-graphics: test_pokemon_graphics.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_pokemon_graphics.cpp $(CORE) -o $@

test-input: test_input.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_input.cpp $(CORE) -o $@

.PHONY: test-input-sdl
test-input-sdl: test_input.cpp Display.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) -DGB_WITH_SDL test_input.cpp Display.cpp $(CORE) $$(pkg-config --cflags --libs sdl3) -o test-input-sdl-bin
	SDL_VIDEODRIVER=dummy ./test-input-sdl-bin

test-timer: test_timer.cpp Timer.cpp Timer.h
	$(CXX) $(CXXFLAGS) test_timer.cpp Timer.cpp -o $@

test-timer-bus: test_timer_bus.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_timer_bus.cpp $(CORE) -o $@

test-halt: test_halt.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_halt.cpp $(CORE) -o $@

test-battery: test_battery.cpp Cartridge.cpp Cartridge.h
	$(CXX) $(CXXFLAGS) test_battery.cpp Cartridge.cpp -o $@

test-audio: test_audio.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_audio.cpp $(CORE) -o $@

.PHONY: test-audio-sdl
test-audio-sdl: test_audio.cpp AudioOutput.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) -DGB_WITH_SDL test_audio.cpp AudioOutput.cpp $(CORE) $$(pkg-config --cflags --libs sdl3) -o test-audio-sdl-bin
	SDL_AUDIODRIVER=dummy ./test-audio-sdl-bin
