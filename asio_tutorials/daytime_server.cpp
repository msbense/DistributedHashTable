#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

std::string make_dt_str() {
    using namespace std;
    time_t now = time(0);
    return ctime(&now);
}

int main(int argc, char **argv) {
    try {

        boost::asio::io_service io;
        std::cout << "here 0" << std::endl;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 13));  
        std::cout << "here 1" << std::endl;
        while (true) {
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::cout << "here 2" << std::endl;

            std::string s = make_dt_str();
            boost::system::error_code error;
            std::cout << "here 3" << std::endl;
            boost::asio::write(socket, boost::asio::buffer(s), error);
        }
    }
    catch (std::exception e) {
        std::cout << e.what() << std::endl;
    }
}