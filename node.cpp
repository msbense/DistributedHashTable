#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>
#include "tcp/tcp_connection.cpp"

using boost::asio::ip::tcp;

class Node {

    public:
    Node(boost::asio::io_service& io_service, int port) : acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

    private:
    void start_accept() {
        tcp_connection::pointer connection = tcp_connection::create(acceptor.get_io_service());
        acceptor.async_accept(connection->socket(), boost::bind(&Node::handle_accept, this, connection, 
                                boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer connection, const boost::system::error_code& error) {
        if (!error) {
            tcp::socket& socket = connection->socket();
            boost::array<char, 128> buf;
            boost::system::error_code error;
            size_t len = socket.read_some(boost::asio::buffer(buf), error);
            if (error) { throw error; }
            std::string request(buf.begin(), buf.end());
            std::string response(get_response(request));
            connection->start(response);
        }
        else { throw error; }
        start_accept();
    }

    enum request_option {GET_NODES, GET, PUT};
    typedef struct { request_option type; std::string content; } request_t;

    std::string get_response(std::string request_str) {
        request_t request(parse_request(request_str));

        return " ";
    }

    request_t parse_request(std::string request_str) {
        return request_t { request_option::GET_NODES, " " };
    }

    boost::unordered_map<int, std::string> map;
    tcp::acceptor acceptor;
};

int main(int argc, char **argv) {
   try { 
        boost::asio::io_service io_service;
        Node n(io_service, 13);

        io_service.run();
   }
   catch (std::exception& e) {
       std::cout << e.what() << std::endl;
   }
}