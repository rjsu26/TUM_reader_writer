CXX = g++
CXXFLAGS = -std=c++11 -Wall

all: server client

server: server.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp -lrt -pthread

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp -lrt -pthread

clean:
	rm -f server client
