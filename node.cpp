#include <ctime>
#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "tcp_connection.cpp"
#include "map.cpp"

         
using boost::asio::ip::tcp;


template<class V> class Node {

    public:
    Node(boost::asio::io_service& io_service, int port) : acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
        
        // for (int i = 0; i < THREAD_COUNT; i++) 
        //     thread_pool.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));

        start_accept();
    }

    private:
    void start_accept() {
        while (true) {
            tcp_connection::pointer connection = tcp_connection::create(acceptor.get_io_service());
            acceptor.accept(connection->socket());
            std::thread t(&Node::handle_accept, this, connection);
            t.detach();
        }
    }

    void handle_accept(tcp_connection::pointer connection) {
        // std::cerr << "Here2 "<< std::endl;
        // std::thread t(&Node::start_accept, this);
        // t.detach();
        tcp::socket& socket = connection->socket();
        // std::cerr << "Here3 "<< std::endl;
        while (socket.is_open()) {
            // std::cerr << "Here4 "<< std::endl;
            boost::array<char, 64> buf;
            boost::system::error_code error;
            size_t len = socket.read_some(boost::asio::buffer(buf), error);
            if (error) {  
                if (error == boost::asio::error::eof) 
                    return;
                std::cerr << "Error thrown in node.cpp when reading socket: " << error.message() << std::endl;
                throw error; 
            }
            // std::cerr << "Here5 "<< std::endl;

            std::string request(buf.begin(), len);
            std::string response(get_response(request));
            connection->start(response);
        }
        
        // t.join();
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
                    // if (!value) { ret = "0"; } else { ret = "1 " + std::to_string(value); }
                    ret = (!value) ? "0" : ("1 " + std::to_string(value));
                }
                break;
            case 'P':
                {
                    int key = std::stoi(request_str.substr(2));
                    size_t v_idx = request_str.find(" ", 2) + 1;
                    V value = std::stoi(request_str.substr(v_idx));
                    bool res = map.put(key, value);
                    ret = (res == false) ? "0" : "1";
                    // if (!res) 
                    // std::cerr << "PID: " << getpid() <<  " ret: " << ret << std::endl;
                }   
                break;
            default:
                break;
        }
        return ret;
    }

    // const int THREAD_COUNT = 10;
    HashMap<int> map;
    // boost::asio::io_service io_service;
    tcp::acceptor acceptor;
    // boost::thread_pool thread_pool;
};

int main(int argc, char **argv) {
    try { 
        int port = 13;
        if (argc > 1) {
            port = atoi(argv[1]);
        }
        boost::asio::io_service io_service;
        // boost::asio::io_service::work work(io_service);
        Node<int> n(io_service, port);

        io_service.run();
        // io_service.stop();
        
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}