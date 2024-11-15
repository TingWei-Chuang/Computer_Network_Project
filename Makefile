all: client.cpp server.cpp clean
	g++ client.cpp -std=c++17 -o client
	g++ server.cpp -pthread -std=c++17 -o server

clean:
	rm -f client
	rm -f server