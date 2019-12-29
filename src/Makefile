CPPFLAGS=-g -Wall -Werror -std=c++17
LDFLAGS=-g -Wall -Werror -std=c++17

all: cpu clean

cpu: cpu.o cartridge.o
	c++ $(LDFLAGS) -o cpu cpu.o cartridge.o

cpu.o: cpu.cpp
	c++ $(CPPFLAGS) -c cpu.cpp

cartridge.o: cartridge.cpp
	c++ $(CPPFLAGS) -c cartridge.cpp


clean:
	rm *~ *.o \#*
