server:server.cpp TCPServer.cpp
	g++ -g -o $@ $^ -std=c++11 -lpthread

.PHONY:clean
clean:
	rm -f server