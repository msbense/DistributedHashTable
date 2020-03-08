#include <ctime>
#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include "tcp/tcp_connection.cpp"
#include "map.cpp"
         

using boost::asio::ip::tcp;

template<class V> class Node {

    public:
    Node(boost::asio::io_service& io_service, int port) : acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

    private:
    void start_accept() {
        std::cerr << "Listening..." << std::endl;
        while (true) {
            tcp_connection::pointer connection = tcp_connection::create(acceptor.get_io_service());
        // acceptor.accept(connection->socket(), boost::bind(&Node::handle_accept, this, connection, 
                                // boost::asio::placeholders::error));
            acceptor.accept(connection->socket());
            if (fork() == 0) {
                handle_accept(connection);
                return;
            }
        }
    }

    void continue_accept() {
        tcp_connection::pointer connection = tcp_connection::create(acceptor.get_io_service());
        acceptor.async_accept(connection->socket(), boost::bind(&Node::handle_accept, this, connection, 
                                boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer connection /*, const boost::system::error_code& error */) {
        // if (!error) {
        tcp::socket& socket = connection->socket();
        
        boost::array<char, 64> buf;
        boost::system::error_code error;
        size_t len = socket.read_some(boost::asio::buffer(buf), error);
        if (error)  
            throw error; 

        std::string request(buf.begin(), len);
        std::cerr << "Request: { " << request << " }" << std::endl;

        std::string response(get_response(request));
        std::cerr << "Response: { " << response << " }" << std::endl;
        
        connection->start(response);
        // }
        // else 
        // throw error;
        
        // continue_accept();
    }

    std::string get_response(std::string request_str) {
        std::string ret = "";

        if (request_str.length() < 1)
            return ret;
        char type = request_str[0];
        switch (type) {
            case 'G':
                {
                    int key = std::stoi(request_str.substr(2));
                    V value = map.get(key);
                    // std::cerr << "Key: { " << key << " }, Value: { " << value << " }" << std::endl;
                    if (!value) { ret = "0"; } else { ret = "1 " + std::to_string(value); }
                    ret = (!value) ? "0" : ("1 " + std::to_string(value));
                }
                break;
            case 'P':
                {
                    int key = std::stoi(request_str.substr(2));
                    size_t v_idx = request_str.find(" ", 2) + 1;
                    V value = std::stoi(request_str.substr(v_idx));
                    bool res = map.put(key, value);
                    // std::cerr << "Key: { " << key << " }, Value: { " << value << " }, " << "Result: { " << res << " }" << std::endl;
                    ret = (res == false) ? "0" : "1";
                }   
                break;
            default:
                break;
        }
        return ret;
    }

    std::vector<std::string> all_nodes;
    HashMap<int> map;
    tcp::acceptor acceptor;
};

int main(int argc, char **argv) {
    try { 
        int port = 13;
        if (argc > 1) {
            port = atoi(argv[1]);
        }
        boost::asio::io_service io_service;
        Node<int> n(io_service, port);

        io_service.run();
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}