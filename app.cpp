#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <random>
#include <vector>
#include <mutex>
#include <thread>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include "log.cpp"
// #include "tbb/task_group.h"

namespace fs = boost::filesystem;
using boost::asio::ip::tcp;

// Parameters! :) 
const int REPLICATION = 2;
const int GET_PROBABILITY = 60;
const int MULTIPUT_PROBABILITY = 20;
int NUM_OPERATIONS = 100; //per thread
int NUM_THREADS = 8;
int KEY_RANGE = 100;
int VALUE_RANGE = 1000;

// Data :D 
int successful_puts = 0;
int unsuccessful_puts = 0;
int successful_multiputs = 0;
int unsuccessful_multiputs = 0;
int successful_gets = 0;
int unsuccessful_gets = 0;

//Network stuff
enum operation_type { GET=0, PUT=1, MULTIPUT=2 };
typedef struct {
    int node_id;
    std::string host;
    std::string port;
} node_info;

struct connection_info_t {
    tcp::socket *socket;
    std::recursive_mutex *mutex;
    int node_id; 
    ~connection_info_t() {
        delete socket;
        delete mutex;
    }
};
typedef struct connection_info_t connection_info;

void print_results(long);
connection_info *connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes, boost::ptr_vector<connection_info> &open_connections);
bool parse_response(std::string s, operation_type optype);
std::vector<node_info> load_node_info(void);
void print_nodes_info(std::vector<node_info> nodes);
void make_transactions(boost::asio::io_service &io, std::vector<node_info> nodes_info, int n_ops);
int transaction(boost::asio::io_service &io, std::vector<node_info> nodes_info, boost::ptr_vector<connection_info> &open_connections);
int get(boost::asio::io_service &io, std::vector<node_info> nodes_info, boost::ptr_vector<connection_info> &open_connections);
int put(boost::asio::io_service &io, std::vector<node_info> nodes_info, boost::ptr_vector<connection_info> &open_connections, int n);
void send_message(connection_info* c, std::string m);
std::string receive_message(connection_info* c);
int node_for_key(int key, std::vector<node_info> nodes_info);

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
            std::thread t([&]{ make_transactions(io, nodes_info, NUM_OPERATIONS); });
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
    
}

void make_transactions(boost::asio::io_service &io, std::vector<node_info> nodes_info, int n_ops) {
    boost::ptr_vector<connection_info> open_connections;
    for (int i = 0; i < n_ops; i++) {
        int result = 0;
        while (result == 0) {
            result = transaction(io, nodes_info, open_connections);
        }
        if (result < 0) {
            return;
        }
    }
    open_connections.clear();
}

int transaction(boost::asio::io_service &io, std::vector<node_info> nodes_info, boost::ptr_vector<connection_info> &open_connections) {
    operation_type optype = get_operation();
    int result = 0;
    switch(optype) {
        case GET:
            result = get(io, nodes_info, open_connections);
            break;
        case PUT:
            result = put(io, nodes_info, open_connections, 1);
            break;
        case MULTIPUT:
            result = put(io, nodes_info, open_connections, 3);
            break;
    }
    return result;
}

int get(boost::asio::io_service &io, std::vector<node_info> nodes_info, boost::ptr_vector<connection_info> &open_connections) {
    int result = 0;

    int key = std::rand() % KEY_RANGE;
    std::string request_str = "G " + std::to_string(key);
    int node_id = node_for_key(key, nodes_info);
    connection_info* con = connect_to_node(io, key, nodes_info, open_connections);
    // con->mutex->lock();
    send_message(con, request_str);
    std::string response = receive_message(con);
    // con->mutex->unlock();
    result = parse_response(response, GET);

    return result;
}

int put(boost::asio::io_service &io, std::vector<node_info> nodes_info, boost::ptr_vector<connection_info> &open_connections, int n) {
    //Decide on an operation
    operation_type optype = get_operation();
    std::vector<std::pair<int, int>> key_values;
    for (int i = 0; i < n; i++) {
        int key = std::rand() % KEY_RANGE;
        int value = std::rand() % VALUE_RANGE;
        for (int j = 0; j < key_values.size(); j++) {
            if (key_values[j].first == key) continue;
        }
        key_values.push_back(std::make_pair(key, value));
    }
    std::sort(key_values.begin(), key_values.end(), 
        [](std::pair<int, int> &p1, std::pair<int, int> &p2) {
            return p1.first > p2.first;
        });

    std::vector<std::pair<connection_info*, std::string>> operations;
    std::vector<std::pair<connection_info*, int>> server_side_locks;
    bool transaction_failed = false;
    std::for_each(key_values.begin(), key_values.end(), [&](std::pair<int, int> &p) {
        thread_print("Setting up set: " + std::to_string(p.first) + " " + std::to_string(p.second));
    });
    std::for_each(key_values.begin(), key_values.end(), [&](std::pair<int, int> &p) {
        if (!transaction_failed) {
            thread_print("Setting up " + std::to_string(p.first) + " " + std::to_string(p.second));
            //lock client side connections
            //lock guard around locks to avoid deadlock
            
            for (int i = 0; i < REPLICATION; i++) {
                std::string req_str = "P " + std::to_string(p.first) + " " + std::to_string(p.second);
                connection_info* con = connect_to_node(io, p.first + i, nodes_info, open_connections);
                operations.push_back(std::pair<connection_info*, std::string>(con, req_str));
                // thread_print("Locked " + std::to_string(cons[i]->node_id));
                
                // thread_print("Locking " + std::to_string(con->node_id));
                // con->mutex->lock();
                // thread_print("Locked " + std::to_string(con->node_id));
                // client_side_locks.push_back(con);

                if (std::find(server_side_locks.begin(), server_side_locks.end(), std::pair<connection_info*,int>(con, p.first)) == server_side_locks.end()) {
                    server_side_locks.push_back(std::pair<connection_info*, int>(con, p.first));
                    send_message(con, "L " + std::to_string(p.first));
                    std::string response = receive_message(con);
                    if (response.compare("0") == 0) {
                        transaction_failed = true;
                        parse_response(response, (n > 1) ? MULTIPUT : PUT);
                        thread_print("Unlocking " + std::to_string(con->node_id));
                        // con->mutex->unlock();
                        server_side_locks.erase(server_side_locks.end() - 1);
                        break;
                    }
                }
            }
        }
    });
    //unlock all server-side locks, return and try again
    if (transaction_failed) {
        thread_print("Transaction failed");
        for (auto p = server_side_locks.begin(); p < server_side_locks.end(); p++) {
            send_message(p->first, "U " + std::to_string(p->second));
        }
        // for (auto c = client_side_locks.begin(); c < client_side_locks.end(); c++) {
        //     thread_print("Unlocking " + std::to_string((*c)->node_id));
        //     (*c)->mutex->unlock();
        // }
        
        return 0;
    }
    // m.unlock();

    //Perform operations
    std::for_each(operations.begin(), operations.end(), 
        [&](std::pair<connection_info*, std::string> &p) {
            send_message(p.first, p.second);
            std::string response = receive_message(p.first);
            parse_response(response, (n == 1) ? PUT : MULTIPUT);
        });
    
    
    //Unlock connections (server-side locks are unlocked upon finish)
    std::for_each(operations.begin(), operations.end(), 
        [&](std::pair<connection_info*, std::string> &p) {
            int key = std::stoi(p.second.substr(2));
            // send_message(p.first, "U " + std::to_string(key));
            thread_print("Unlocking " + std::to_string(p.first->node_id));
            // p.first->mutex->unlock();
        });
    
    return 1;
}

int node_for_key(int key, std::vector<node_info> nodes_info) {
    return key % nodes_info.size();
}

void send_message(connection_info *connection, std::string message) {
    thread_print("To " + std::to_string(connection->node_id) + " " + message);
    message.append("\n");
    connection->socket->write_some(boost::asio::buffer(message));
    thread_print("done");
}

std::string receive_message(connection_info* connection) {
    boost::array<char, 128> buf;
    boost::system::error_code error;
    size_t len = connection->socket->read_some(boost::asio::buffer(buf), error);

    if (error == boost::asio::error::eof) {
        std::stringstream sstr;
        sstr << "EOF when reading from node " << connection->node_id << std::endl;
        std::cerr << sstr.str();
    }
    else if (error) {
        std::stringstream sstr;
        sstr << "Error app.cpp: " << error.message() << " when connecting to node " << 
            connection->node_id << std::endl;
        std::cerr << sstr.str();
    }
    std::string res(buf.data(), len); 
    thread_print("From " + std::to_string(connection->node_id) + " " + res);
    return res;
}

operation_type get_operation() {
    if (std::rand() % 100 < GET_PROBABILITY) {
        return operation_type::GET;
    }
    else {
        if (std::rand() % (100 - GET_PROBABILITY) < MULTIPUT_PROBABILITY) {
            return operation_type::MULTIPUT;
        }
        else {
            return operation_type::PUT;
        }
    }
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
    std::cout << "+G:  " << successful_gets << std::endl;
    std::cout << "-G:  " << unsuccessful_gets << std::endl;
    std::cout << "+P:  " << successful_puts << std::endl;
    std::cout << "-P:  " << unsuccessful_puts << std::endl;
    std::cout << "+MP: " << successful_multiputs << std::endl;
    std::cout << "-MP: " << unsuccessful_multiputs << std::endl;

    std::cout << "Duration: " << duration << " microseconds" << std::endl;
    double throughput = (NUM_OPERATIONS * NUM_THREADS) / (duration / 1E6);
    std::cout << "Throughput: " << throughput << " operations / second" << std::endl;
    std::cout << "Latency: " << 1 / throughput << " seconds / operation" << std::endl; 
}

//returns a socket to the node responsible for that key
connection_info *connect_to_node(boost::asio::io_service& io, int key, std::vector<node_info> nodes, boost::ptr_vector<connection_info> &open_connections) {
    // static std::mutex mtx;
    // const std::lock_guard<std::mutex> lock(mtx);
    
    tcp::resolver resolver(io);
    int node_id = (key % nodes.size());
    node_info node = nodes[node_id];
    tcp::resolver::query query(node.host, node.port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    
    thread_print("Creating connection");
    for (int i = 0; i < open_connections.size(); i++) {
        if (endpoint_iterator->endpoint() == open_connections[i].socket->remote_endpoint()) {
            return &open_connections[i];
        }
    }

    connection_info *connection = new connection_info();
    connection->socket = new tcp::socket(io);

    connection->mutex = new std::recursive_mutex();
    connection->node_id = node_id;
    boost::asio::connect(*(connection->socket), endpoint_iterator);
    // boost::asio::ip::tcp::no_delay option(true);
    // connection->socket->set_option(option);
    open_connections.push_back(connection);
    thread_print("Connection created");
    return connection;
}

bool parse_response(std::string response_string, operation_type optype) {
    switch (optype) {
        case GET:
            if (response_string[0] == '0') {
                unsuccessful_gets++;
                return false;
            }
            else successful_gets++;
            break;
        case PUT:
            if (response_string[0] == '0') {
                unsuccessful_puts++;
                return false;
            }
            else successful_puts++;
            break;
        case MULTIPUT:
            if (response_string[0] == '0') {
                unsuccessful_multiputs++;
                return false;
            }
            else successful_multiputs++;
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