#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <string>
#include <random>

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
void parse_response(boost::array<char, 128>& buffer, size_t len, operation_type o);

int main(int argc, char *argv[]) {
    try {
        if (argc > 3) {
            NUM_OPERATIONS = atoi(argv[1]);
            KEY_RANGE = atoi(argv[2]);
            VALUE_RANGE = atoi(argv[3]);
        }

        boost::asio::io_service io;
        tcp::resolver resolver(io);
        tcp::resolver::query query("localhost", "13");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            tcp::socket socket(io);
            boost::asio::connect(socket, endpoint_iterator);
            boost::system::error_code error;

            std::string to_server = "";
            operation_type optype;
            
            if (std::rand() % 100 < GET_PROBABILITY) {
                optype = operation_type::GET;
                to_server = to_server + "G ";
                int key = std::rand() % KEY_RANGE;
                to_server = to_server + std::to_string(key);
            }
            else {
                optype = operation_type::PUT;
                to_server = to_server + "P ";
                int key = std::rand() % KEY_RANGE;
                int value = std::rand() % VALUE_RANGE;
                to_server = to_server + std::to_string(key) + " " + std::to_string(value);
            }
            
            std::cout << "Request: { " << to_server << " }" << std::endl;
            socket.write_some(boost::asio::buffer(to_server));
            
            boost::array<char, 128> buf;
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

    std::cout << std::endl;
    std::cout << "Results (+ = successful): " << std::endl;
    std::cout << "+G: " << successful_gets << std::endl;
    std::cout << "-G: " << unsuccessful_gets << std::endl;
    std::cout << "+P: " << successful_puts << std::endl;
    std::cout << "-P: " << unsuccessful_puts << std::endl;
     
}

void parse_response(boost::array<char, 128>& buffer, size_t len, operation_type o) {
    std::string response_string(buffer.data(), len);
    std::cout << "Response: { " << response_string << " }" << std::endl;
    switch (o) {
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