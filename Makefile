all: netcode

netcode: daytime_client.cpp daytime_server.cpp
	g++ daytime_client.cpp -O3 -o ./obj64/daytime_client.o -lboost_system -lboost_thread -lpthread
	g++ daytime_server.cpp -O3 -o ./obj64/daytime_server.o -lboost_system -lboost_thread -lpthread

async: daytime_server_async.cpp
	g++ daytime_server_async.cpp -O3 -o ./obj64/daytime_server_async.o -lboost_system -lboost_thread -lpthread

clean:
	rm -rf ./obj64/*