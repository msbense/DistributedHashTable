all: node app

node: node.cpp tcp/tcp_connection.cpp map.cpp
	g++ -std=c++11 node.cpp tcp/tcp_connection.cpp map.cpp -O3 -o ./obj64/node.o -lboost_system -lboost_thread -lpthread

app: app.cpp
	g++ -std=c++11 app.cpp -O3 -o ./obj64/app.o -lboost_system -lboost_thread -lpthread

debug: debug.cpp
	g++ -std=c++11 debug.cpp -O3 -o ./obj64/debug.o -lboost_system -lboost_thread -lpthread

clean:
	rm -rf ./obj64/*