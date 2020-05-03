#include <ctime>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include "tcp_connection.cpp"
#include "map.cpp"
#include "log.cpp"
         
using boost::asio::ip::tcp;

//TODO two phase commit node side
template<class V> class Node {

    public:
    Node(boost::asio::io_service& io_service, int port) 
        : acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

    private:
    void start_accept() {
        std::vector<std::thread> threads;
        std::cerr << "Listening..." << std::endl;
        
        while (true) {
            tcp_connection::pointer connection = tcp_connection::create(acceptor.get_io_service());
            acceptor.accept(connection->socket());
            std::cerr << "Accepted Connection" << std::endl;
            std::thread t(&Node::handle_accept, this, connection);
            threads.push_back(std::move(t));
        }
        
        auto join = [](std::thread &t) { t.join(); };
        std::for_each(threads.begin(), threads.end(), join);
    }

    void handle_accept(tcp_connection::pointer connection) {
        tcp::socket& socket = connection->socket();
        while (socket.is_open()) {
            boost::system::error_code error;
            size_t len = 0;
            try {
                len = boost::asio::read_until(socket, connection->buffer, '\n', error);
            }
            catch (std::exception e) {
                thread_print(e.what());
            }
            if (error) {  
                if (error == boost::asio::error::eof) { 
                        break;
                }
                else {
                    // std::cerr << "Error thrown in node.cpp when reading socket: " << error.message() << std::endl;
                    // std::cerr << "Closing connection..";
                    return;
                }
            }
            std::string request(boost::asio::buffer_cast<const char*>(connection->buffer.data()), len);
            std::string response(get_response(request));
            if (response.compare("") != 0)
                connection->start(response + "\n");
            connection->buffer.consume(len);
        }
        // print("Socket closed");
        std::cerr << "Socket Closed" << std::endl;
        
    }

    std::string get_response(std::string request_str) {
        
        // thread_print("requesting " + request_str);
        std::string ret = "";

        if (request_str.length() < 1)
            return ret;
        char type = request_str[0];
        boost::erase_all(request_str, "\n");
        switch (type) {
            case 'G':
                {
                    int key = std::stoi(request_str.substr(2));
                    bool res = map.try_lock(key);
                    ret = (res) ? "1 " + std::to_string(map.get(key)) : "0";
                    if (res) map.unlock(key);
                    // log("");
                }
                break;
            case 'P':
                {
                    int key = std::stoi(request_str.substr(2));
                    size_t v_idx = request_str.find(" ", 2) + 1;
                    V value = std::stoi(request_str.substr(v_idx));
                    map.put(key, value);
                    map.unlock(key);
                    log(request_str);
                    // ret = "1";
                }   
                break;
            case 'L':
                {
                    std::vector<std::string> tokens;
                    boost::split(tokens, request_str, boost::is_any_of(" "));
                    std::vector<int> locked_keys;
                    for (int i = 1; i < tokens.size(); i++) {
                        int key = std::stoi(tokens[i]);
                        bool res = map.try_lock(key);
                        ret = (res) ? "1" : "0";
                        if (!res) {
                            for (int j = 0; j < locked_keys.size(); j++)
                                map.unlock(locked_keys[j]);
                            thread_print("Failed");
                        }
                        else 
                            locked_keys.push_back(key);
                    }
                    log(request_str + " 1");
                }
                break;
            case 'U':
                {
                    std::vector<std::string> tokens;
                    boost::split(tokens, request_str, boost::is_any_of(" "));
                    for (int i = 1; i < tokens.size(); i++) {
                        int key = std::stoi(tokens[i]);
                        map.unlock(key);
                    }
                    log(request_str);
                }
                break;
            default:
                break;
        }
        // thread_print("returning " + ret);
        return ret;
    }

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