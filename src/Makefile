CPPFLAGS=-g -Wall -Werror -std=c++17
LDFLAGS=-g -Wall -Werror -std=c++17

all: main clean

main: main.o cpu.o cartridge.o mapper.o
	c++ $(LDFLAGS) -o main main.o cpu.o cartridge.o mapper.o

main.o: main.cpp
	c++ $(CPPFLAGS) -c main.cpp

cpu.o: cpu.cpp
	c++ $(CPPFLAGS) -c cpu.cpp

cartridge.o: cartridge.cpp 
	c++ $(CPPFLAGS) -c cartridge.cpp

mapper.o:
	c++ $(CPPFLAGS) -c mapper.cpp


clean:
	rm *~ *.o \#*
