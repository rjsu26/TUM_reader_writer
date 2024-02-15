CXX = g++
CXXFLAGS = -std=c++11 -Wall

all: server client

server: server.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp -lrt -pthread -g

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp -lrt -pthread -g 

clean:
	rm -f server client
