VPATH = ./
CXX = g++
TARGET = sample_main
CPPFLAGS = -Wall -std=c++11 -g

$(TARGET): sample_main.o lib_sample.o
	$(CXX) $^ -o $@ $(CPPFLAGS)
sample_main.o: sample_main.cpp
	$(CXX) -c $^ $(CPPFLAGS)
lib_sample.o: lib_sample.cpp
	$(CXX) -c $^ $(CPPFLAGS)

clean:
	rm -f *.o $(TARGET)


