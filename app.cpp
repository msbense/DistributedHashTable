#include <iostream>
#include <fstream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <string>
#include <random>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
using boost::asio::ip::tcp;

// Parameters! :) 
const int GET_PROBABILITY = 60;
int NUM_OPERATIONS = 5;
int KEY_RANGE = 100;
int VALUE_RANGE = 1000;

// Data :D 
int successful_puts = 0;
int unsuccessful_puts = 0;
int successful_gets = 0;
int unsuccessful_gets = 0;

enum operation_type { GET=0, PUT=1  };
typedef struct {
    int node_id;
    std::string host;
    std::string port;
} node_info;

void print_results(void);
tcp::socket connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes);
void parse_response(boost::array<char, 128>& buffer, size_t len, operation_type optype);
std::vector<node_info> load_node_info(void);

int main(int argc, char *argv[]) {
    try {

        std::vector<node_info> nodes_info = load_node_info();
        
        if (argc > 2) {
            NUM_OPERATIONS = atoi(argv[1]);
            KEY_RANGE = atoi(argv[2]);
        }

        boost::asio::io_service io;
        
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            
            std::string to_server = "";
            operation_type optype;
            
            int key = std::rand() % KEY_RANGE;
            if (std::rand() % 100 < GET_PROBABILITY) {
                optype = operation_type::GET;
                to_server = to_server + "G ";
                to_server = to_server + std::to_string(key);
            }
            else {
                optype = operation_type::PUT;
                to_server = to_server + "P ";
                int value = std::rand() % VALUE_RANGE;
                to_server = to_server + std::to_string(key) + " " + std::to_string(value);
            }
            
            // std::cout << "Connect: "
            std::cout << "Request (" << nodes_info[key % nodes_info.size()].host <<  ") : { " << to_server << " }" << std::endl;
            tcp::socket socket = connect_to_node(io, key, nodes_info);
            socket.write_some(boost::asio::buffer(to_server));
            
            boost::array<char, 128> buf;
            boost::system::error_code error;
            size_t len = socket.read_some(boost::asio::buffer(buf), error);
            if (error == boost::asio::error::eof) {
                std::cout << "EOF" << std::endl;
                return 0;
            }
            else if (error) {
                throw error;
            }

            parse_response(buf, len, optype);
        }

    }
    catch (std::exception& e) {

        std::cerr << e.what() << std::endl;
    }

    print_results();
}

std::vector<node_info> load_node_info() {
    std::ifstream in("node_info.txt");
    std::stringstream sstr;
    sstr << in.rdbuf();
    std::string ni_str(sstr.str());

    std::vector<node_info> ret;
    std::vector<std::string> node_strs;
    int idx = 0;
    while ((idx = ni_str.find("\n")) != -1) {
        node_strs.push_back(ni_str.substr(0, idx));
        ni_str = ni_str.substr(idx + 1, ni_str.length() - idx); //abce:32
    } 
    for (int i = 0; i < node_strs.size(); i++) {
        node_info ni;
        int colon_idx = node_strs[i].find(":");
        ni.host = node_strs[i].substr(0, colon_idx);
        ni.port = node_strs[i].substr(colon_idx + 1, node_strs[i].length() - colon_idx);
        ni.node_id = i;
        ret.push_back(ni);
    }

    return ret;
}

void print_results() {
    std::cout << std::endl;
    std::cout << "Results (+ = successful): " << std::endl;
    std::cout << "+G: " << successful_gets << std::endl;
    std::cout << "-G: " << unsuccessful_gets << std::endl;
    std::cout << "+P: " << successful_puts << std::endl;
    std::cout << "-P: " << unsuccessful_puts << std::endl;
}

//returns a socket to the node responsible for that key
tcp::socket connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes) {
    tcp::resolver resolver(io);
    int node_id = (key % nodes.size());
    node_info node = nodes[node_id];
    tcp::resolver::query query(node.host, node.port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::socket socket(io);
    boost::asio::connect(socket, endpoint_iterator);
    return socket;
}

void parse_response(boost::array<char, 128>& buffer, size_t len, operation_type optype) {
    std::string response_string(buffer.data(), len);
    std::cout << "Response: { " << response_string << " }" << std::endl;
    switch (optype) {
        case GET:
            if (response_string[0] == '0') unsuccessful_gets++;
            else successful_gets++;
            break;
        case PUT:
            if (response_string[0] == '0') unsuccessful_puts++;
            else successful_puts++;
            break;
        default:
            break;
    }
}