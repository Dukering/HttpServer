SOURCE  := $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

TARGET  := Server
CC      := g++
LIBS    := -lpthread 
CFLAGS  := -std=c++11 -g -Wall 
CXXFLAGS:= $(CFLAGS)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
	
clean :
	rm -fr *.o Server




