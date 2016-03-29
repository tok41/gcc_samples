VPATH = ./
CXX = g++
TARGET1 = cond_var_sample_3
## For Linux
#CPPFLAGS = -Wall -pthread -std=c++11 -g
CPPFLAGS = -pthread -std=c++11 -g
## For Mac
#CPPFLAGS = -Wall -std=c++11 -stdlib=libc++ -g

$(TARGET1): $(TARGET1).cpp
	$(CXX) $^ -o $@ $(CPPFLAGS)
# $(TARGET2): test_client.o device_server.o
# #	$(CXX) $^ -o $@ $(CPPFLAGS)

#server_class.o: device_server.cpp
#	$(CXX) -c $^ $(CPPFLAGS)
#server_test.o: server_test.cpp
#	$(CXX) -c $^ $(CPPFLAGS)
#test_client.o: test_client.cpp
#	$(CXX) -c $^ $(CPPFLAGS)

clean:
	rm -f *.o $(TARGET1) $(TARGET2)


