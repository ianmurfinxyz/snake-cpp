LDLIBS = -lSDL2 -lm -lGLX_mesa
CXXFLAGS = -Wall -std=c++17 -fno-exceptions -DNDEBUG #-g

snake : snake.cpp
	$(CXX) $(CXXFLAGS) -o $@ snake.cpp $(LDLIBS)

.PHONY: clean
clean:
	rm snake *.o
