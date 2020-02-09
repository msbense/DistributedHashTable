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
        tcp_connection::pointer connection = tcp_connection::create(acceptor.get_io_service());
        acceptor.async_accept(connection->socket(), boost::bind(&Node::handle_accept, this, connection, 
                                boost::asio::placeholders::error));
        std::cerr << "Now accepting commands async" << std::endl;
    }

    void handle_accept(tcp_connection::pointer connection, const boost::system::error_code& error) {
        if (!error) {
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
            
            // boost::asio::const_buffer res_buf = boost::asio::buffer(response);
            connection->start(response);
        }
        else 
            throw error;
        
        start_accept();
    }

        // enum response_code {SUCCESS = 1, FAIL = 0 /*, LIST_NODES */};
        // typedef struct {
        //     response_code code;
        //     V value;
        // } response_t;

    std::string get_response(std::string request_str) {
        std::string ret = "";
        // ret.code = response_code::FAIL;

        if (request_str.length() < 1)
            return ret;
        char type = request_str[0];
        switch (type) {
            // case '?':
            //     for (int i = 0; i < all_nodes.size(); i++)
            //         ret += all_nodes[i] + ",";
            //     break;
            case 'G':
                {
                    int key = std::stoi(request_str.substr(2));
                    V value = map.get(key);
                    std::cerr << "Key: { " << key << " }, Value = { " << value << " }" << std::endl;
                    if (!value) {
                        ret = "0";
                        // ret.code = response_code::FAIL;
                    }
                    else {
                        ret = "1 " + std::to_string(value);
                        // ret.code = response_code::SUCCESS;
                        // ret.value = value;
                    }
                }
                break;
            case 'P':
                {
                    int key = std::stoi(request_str.substr(2));
                    size_t v_idx = request_str.find(" ", 2) + 1;
                    V value = std::stoi(request_str.substr(v_idx));
                    bool res = map.put(key, value);
                    if (res == false) {
                        ret = "0";
                        // ret.code = response_code::FAIL;
                    }
                    else {
                        ret = "1";
                        // ret.code = response_code::SUCCESS;
                    }
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
        boost::asio::io_service io_service;
        int port = 13;
        Node<int> n(io_service, port);

        io_service.run();
   }
   catch (std::exception& e) {
       std::cout << e.what() << std::endl;
   }
}