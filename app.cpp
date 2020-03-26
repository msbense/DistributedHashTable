#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <random>
#include <vector>
#include <mutex>
#include <thread>
#include <set>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace fs = boost::filesystem;
using boost::asio::ip::tcp;

// Parameters! :) 
const int GET_PROBABILITY = 60;
const int MULTIPUT_PROBABILITY = 0;
int NUM_OPERATIONS = 100;
int NUM_THREADS = 8;
int KEY_RANGE = 100;
int VALUE_RANGE = 1000;

// Data :D 
int successful_puts = 0;
int unsuccessful_puts = 0;
int successful_gets = 0;
int unsuccessful_gets = 0;

//Network stuff
enum operation_type { GET=0, PUT=1 };
typedef struct {
    int node_id;
    std::string host;
    std::string port;
} node_info;

struct connection_info_ {
    tcp::socket *socket;
    std::recursive_mutex *mutex;
    int node_id; 
    ~connection_info_() {
        delete socket;
        delete mutex;
    }
};
typedef struct connection_info_ connection_info;
boost::ptr_vector<connection_info> open_connections;

void print_results(long);
connection_info *connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes);
bool parse_response(boost::array<char, 128>& buffer, size_t len, operation_type optype);
std::vector<node_info> load_node_info(void);
void print_nodes_info(std::vector<node_info> nodes);
void make_transactions(boost::asio::io_service &io, std::vector<node_info> nodes_info);
int transaction(boost::asio::io_service &io, std::vector<node_info> nodes_info);

operation_type get_operation();
std::vector<std::pair<int,int>> generate_keys_val_pairs(operation_type optype);
std::vector<std::string> build_request_strs(operation_type optype, std::vector<std::pair<int, int>> key_values);

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
        
        std::vector<std::thread> threads;
        for (int i = 0; i < NUM_THREADS; i++) {
            std::thread t([&]{ make_transactions(io, nodes_info); });
            threads.push_back(std::move(t));
        }
        std::for_each(threads.begin(), threads.end(), 
            [](std::thread &t) { 
                t.join(); 
            });
        
        auto t2 = std::chrono::high_resolution_clock::now();
        
        long duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
        print_results(duration);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    open_connections.clear();
}

void make_transactions(boost::asio::io_service &io, std::vector<node_info> nodes_info) {
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        int result = 0;
        while (result == 0) {
            result = transaction(io, nodes_info);
        }
        if (result < 0) {
            return;
        }
    }
}

//TODO two phase commit client side
int new_transaction(boost::asio::io_service &io, std::vector<node_info> nodes_info) {

    //Decide on an operation
    operation_type optype = get_operation();
    std::vector<std::pair<int, int>> key_values = generate_keys_val_pairs(optype);
    // std::vector<std::string> request_strs = build_request_strs(optype, key_values);

    //Build connection strings, get connection_infos to the correct nodes
    std::vector<std::pair<connection_info*, std::string>> operations;
    std::for_each(key_values.begin(), key_values.end(), 
        [&](std::pair<int, int> &p) {
            std::string req_str = build_request_str(optype, p.first, p.second);
            int node_id1 = p.first % nodes_info.size();
            int node_id2 = (node_id1 + 1) % nodes_info.size(); 
            connection_info* c1 = new_connect_to_node(io, node_id1, nodes_info);
            connection_info* c2 = new_connect_to_node(io, node_id1, nodes_info);
            operations.push_back(std::pair(c1, req_str));
            operations.push_back(std::pair(c2, req_str));
        });

    //Lock client-side and server-side connections
    std::for_each(operations.begin(), operations.end(), 
        [&](std::pair<connection_info*, std::string> &p) {
            p.first->mutex->lock();
            p.first->socket->write_some(boost::asio::buffer("Lock "));
        });
    
    //Send messages
    std::for_each(operations.begin(), operations.end(), 
        [&](std::pair<connection_info*, std::string> &p){
            p.first->socket->write_some(boost::asio::buffer(p.second));
            boost::array<char, 128> buf;
            boost::system::error_code error;
            size_t len = p.first->socket->read_some(boost::asio::buffer(buf), error);
        });

    std::for_each(operations.begin(), operations.end(), 
        [&](std::pair<connection_info*, std::string> &p) {
            p.first->mutex->unlock();
            p.first->socket->write_some(boost::asio::buffer("Unlock "));
        });
}

//TODO write function
connection_info* new_connect_to_node(boost::asio::io_service &io, int node, std::vector<node_info> nodes) {}

//TODO write function
operation_type get_operation() {}

//TODO write function
std::vector<std::pair<int,int>> generate_keys_val_pairs(operation_type optype) {}

//TODO write function
std::string build_request_str(operation_type optype, int key, int value) {}


int transaction(boost::asio::io_service &io, std::vector<node_info> nodes_info) {
    std::string to_server = "";
    operation_type optype;
        
    int key = std::rand() % KEY_RANGE;
    if (std::rand() % 100 < GET_PROBABILITY) {
        optype = operation_type::GET;
        to_server = "G " + std::to_string(key);
    }
    else {
        if (std::rand() % 100 < MULTIPUT_PROBABILITY) {
            // optype = operation_type::MULTIPUT;
            int key2 = std::rand() % KEY_RANGE;
            int key3 = std::rand() % KEY_RANGE;
            int val1 = std::rand() % VALUE_RANGE;
            int val2 = std::rand() % VALUE_RANGE;
            int val3 = std::rand() % VALUE_RANGE;
            std::stringstream sstr;
            sstr << "M " << std::to_string(key) << " " << std::to_string(val1);
            sstr << " " << std::to_string(key2) << " " << std::to_string(val2);
            sstr << " " << std::to_string(key3) << " " << std::to_string(val3);
            to_server = sstr.str();
        }
        else {
            optype = operation_type::PUT;
            int value = std::rand() % VALUE_RANGE;
            to_server = "P " + std::to_string(key) + " " + std::to_string(value);
        }
    }

    connection_info *connection = connect_to_node(io, key, nodes_info);
        
    connection->mutex->lock();
    connection->socket->write_some(boost::asio::buffer(to_server));
    boost::array<char, 128> buf;
    boost::system::error_code error;
    size_t len = connection->socket->read_some(boost::asio::buffer(buf), error);
    connection->mutex->unlock();

    if (error == boost::asio::error::eof) {
        std::stringstream sstr;
        sstr << "EOF when reading from node " << connection->node_id << std::endl;
        std::cerr << sstr.str();
        return -1;
    }
    else if (error) {
        std::stringstream sstr;
        sstr << "Error app.cpp: " << error.message() << " when connecting to node " << 
            connection->node_id << std::endl;
        std::cerr << sstr.str();
        return -2;
    }

    bool result = parse_response(buf, len, optype);
    
    if (result == false)
        return 0;
    else return 1;
    
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
        ni_str = ni_str.substr(idx + 1, ni_str.length() - idx);
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
connection_info *connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes) {
    static std::mutex mtx;
    const std::lock_guard<std::mutex> lock(mtx);
    
    tcp::resolver resolver(io);
    int node_id = (key % nodes.size());
    node_info node = nodes[node_id];
    tcp::resolver::query query(node.host, node.port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    
    for (int i = 0; i < open_connections.size(); i++) {
        if (endpoint_iterator->endpoint() == open_connections[i].socket->remote_endpoint()) {
            return &open_connections[i];
        }
    }
    connection_info *connection = new connection_info();
    connection->socket = new tcp::socket(io);
    connection->mutex = new std::mutex();
    connection->node_id = node_id;
    boost::asio::connect(*(connection->socket), endpoint_iterator);
    open_connections.push_back(connection);
    return connection;
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