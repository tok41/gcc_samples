VPATH = ./
CXX = g++
TARGET1 = server
TARGET1 = client
CPPFLAGS = -Wall -pthread -std=c++11 -g

$(TARGET1): server.o
	$(CXX) $^ -o $@ $(CPPFLAGS)
server.o: server.cpp
	$(CXX) -c $^ $(CPPFLAGS)

$(TARGET2): client.o
	$(CXX) $^ -o $@ $(CPPFLAGS)
client.o: client.cpp
	$(CXX) -c $^ $(CPPFLAGS)

clean:
	rm -f *.o $(TARGET)


