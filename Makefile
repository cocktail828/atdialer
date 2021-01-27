CXX := g++

CXXFLAGS += -Wall -g -std=c++11

OBJS := $(patsubst %.cpp, %.o, $(wildcard *.cpp))
atdialer: clean $(OBJS)
	$(CXX) -o $@ $(OBJS) -lpthread

clean:
	rm -rf *.o atdialer