#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <random>
#include <vector>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace fs = boost::filesystem;
using boost::asio::ip::tcp;

// Parameters! :) 
const int GET_PROBABILITY = 70;
int NUM_OPERATIONS = 100;
int KEY_RANGE = 100;
int VALUE_RANGE = 1000;

// Data :D 
int successful_puts = 0;
int unsuccessful_puts = 0;
int successful_gets = 0;
int unsuccessful_gets = 0;

//Network stuff
enum operation_type { GET=0, PUT=1, TALK=2 };
typedef struct {
    int node_id;
    std::string host;
    std::string port;
} node_info;
boost::ptr_vector<tcp::socket> open_connections;

void print_results(long);
tcp::socket *connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes);
bool parse_response(boost::array<char, 128>& buffer, size_t len, operation_type optype);
std::vector<node_info> load_node_info(void);
void print_nodes_info(std::vector<node_info> nodes);

int main(int argc, char *argv[]) {
    try {

        std::vector<node_info> nodes_info = load_node_info();

        if (argc > 1) {
            NUM_OPERATIONS = atoi(argv[1]);
            KEY_RANGE = atoi(argv[2]);
        }

        boost::asio::io_service io;
        
        print_nodes_info(nodes_info);

        auto t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            
            std::string to_server = "";
            operation_type optype;
            
            int key = std::rand() % KEY_RANGE;
            if (std::rand() % 100 < GET_PROBABILITY) {
                optype = operation_type::GET;
                to_server = "G " + std::to_string(key);
            }
            else {
                optype = operation_type::PUT;
                int value = std::rand() % VALUE_RANGE;
                to_server = "P " + std::to_string(key) + " " + std::to_string(value);
            }
            
            tcp::socket *socket = connect_to_node(io, key, nodes_info);
            socket->write_some(boost::asio::buffer(to_server));
            
            boost::array<char, 128> buf;
            boost::system::error_code error;
            size_t len = socket->read_some(boost::asio::buffer(buf), error);
            if (error == boost::asio::error::eof) {
                std::cout << "EOF" << std::endl;
                return 0;
            }
            else if (error) {
                std::cerr << "Error app.cpp: " << error.message() << std::endl;
                // throw error;
                return -1;
            }

            bool result = parse_response(buf, len, optype);
            if (!result) {
                // goto try_transaction;
            }
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        long duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
        print_results(duration);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    open_connections.clear();
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

void print_results(long duration) {
    std::cout << std::endl;
    std::cout << "Results (+ = successful): " << std::endl;
    std::cout << "+G: " << successful_gets << std::endl;
    std::cout << "-G: " << unsuccessful_gets << std::endl;
    std::cout << "+P: " << successful_puts << std::endl;
    std::cout << "-P: " << unsuccessful_puts << std::endl;

    std::cout << "Duration: " << duration << " microseconds" << std::endl;
    double throughput = NUM_OPERATIONS / (duration / 1E6);
    std::cout << "Throughput: " << throughput << " operations / second" << std::endl;
    std::cout << "Latency: " << 1 / throughput << " seconds / operation" << std::endl; 
}

//returns a socket to the node responsible for that key
tcp::socket *connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes) {
    tcp::resolver resolver(io);
    int node_id = (key % nodes.size());
    node_info node = nodes[node_id];
    tcp::resolver::query query(node.host, node.port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    
    for (int i = 0; i < open_connections.size(); i++)
        if (endpoint_iterator->endpoint() == open_connections[i].remote_endpoint())
            return &open_connections[i];
    
    tcp::socket *socket = new tcp::socket(io);
    boost::asio::connect(*socket, endpoint_iterator);
    open_connections.push_back(socket);
    return socket;
}

bool parse_response(boost::array<char, 128>& buffer, size_t len, operation_type optype) {
    std::string response_string(buffer.data(), len);
    // std::cout << "Response: { " << response_string << " }" << std::endl;
    switch (optype) {
        case GET:
            if (response_string[0] == '0') unsuccessful_gets++;
            else successful_gets++;
            break;
        case PUT:
            if (response_string[0] == '0') {
                unsuccessful_puts++;
                return false;
            }
            else successful_puts++;
            break;
        default:
            break;
    }
    return true;
}

void print_nodes_info(std::vector<node_info> nodes) {
    std::cerr << "Nodes: " << nodes.size() << std::endl;
    for (int i = 0; i < nodes.size(); i++) {
        std::cerr << nodes[i].node_id << " ";
        std::cerr << nodes[i].host << ":";
        std::cerr << nodes[i].port;
        std::cerr << std::endl;
    }
}