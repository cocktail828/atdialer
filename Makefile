CXX := g++

CXXFLAGS += -Wall -g

OBJS := $(patsubst %.cpp, %.o, $(wildcard *.cpp))
atdialer: clean $(OBJS)
	$(CXX) -o $@ $(OBJS) -lpthread

clean:
	rm -rf *.o atdialer