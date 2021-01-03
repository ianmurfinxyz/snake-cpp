LDLIBS = -lSDL2 -lm -lGLX_mesa
CXXFLAGS = -g -Wall -std=c++17 -fno-exceptions

snake : snake.cpp
	$(CXX) $(CXXFLAGS) -o $@ snake.cpp $(LDLIBS)

.PHONY: clean
clean:
	rm snake *.o
