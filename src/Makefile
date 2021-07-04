CPPFLAGS=-g -Wall -Werror -std=c++17
LDFLAGS=-g -Wall -Werror -std=c++17 -L/opt/homebrew/lib -lSDL2

all: main clean

main: main.o cpu.o cartridge.o mapper.o ppu.o gui.o controller.o
	c++ $(LDFLAGS) -o main main.o cpu.o cartridge.o mapper.o ppu.o gui.o controller.o

main.o: main.cpp
	c++ $(CPPFLAGS) -c main.cpp

cpu.o: cpu.cpp
	c++ $(CPPFLAGS) -c cpu.cpp

cartridge.o: cartridge.cpp
	c++ $(CPPFLAGS) -c cartridge.cpp

gui.o: gui.cpp
	c++ $(CPPFLAGS) -c gui.cpp

ppu.o: ppu.cpp cpu.o
	c++ $(CPPFLAGS) -c ppu.cpp

mapper.o:
	c++ $(CPPFLAGS) -c mapper.cpp

controller.o: controller.cpp
	c++ $(CPPFLAGS) -c controller.cpp


clean:
	rm *~ *.o \#*
