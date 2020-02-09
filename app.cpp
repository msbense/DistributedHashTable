#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <string>
#include <random>

using boost::asio::ip::tcp;
const int prob_get = 60;

int main(int argc, char *argv[]) {
    try {
        int num_operations = 5;
        if (argc > 1) {
            num_operations = atoi(argv[1]);
        }

        boost::asio::io_service io;
        tcp::resolver resolver(io);

        tcp::resolver::query query("localhost", "13");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        
        for (int i = 0; i < num_operations; i++) {
            tcp::socket socket(io);
            boost::asio::connect(socket, endpoint_iterator);
            boost::system::error_code error;

            std::string to_server = "";
            
            if (std::rand() % 100 < prob_get) {
                to_server = to_server + "G ";
                int key = std::rand() % 100;
                to_server = to_server + std::to_string(key);
            }
            else {
                to_server = to_server + "P ";
                int key = std::rand() % 100;
                int value = std::rand() % 1000;
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

            std::cout << "Response: { ";
            std::cout.write(buf.data(), len);
            std::cout << " }" << std::endl;
        }

    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}