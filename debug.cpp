#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main(int argc, char *argv[]) {
    try {
        if (argc < 2) {
            std::cout << "Enter command to send to localhost" << std::endl;
            return 1;
        }
        

        boost::asio::io_service io;
        tcp::resolver resolver(io);

        int key = atoi(argv[2]);
        std::string port(std::to_string(13));
        tcp::resolver::query query("localhost", port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::socket socket(io);
        boost::asio::connect(socket, endpoint_iterator);

        boost::system::error_code error;
        std::string to_server = "";
        for (int i = 1; i < argc; i++)
            to_server = to_server + argv[i] + " ";
        std::cout << to_server << std::endl;
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

        std::cout << "Message: { ";
        std::cout.write(buf.data(), len);
        std::cout << " }" << std::endl;
        

    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}