#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;


class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection> {
        public:
        typedef boost::shared_ptr<tcp_connection> pointer;

        static pointer create(boost::asio::io_service& io_service) {
            return pointer(new tcp_connection(io_service));
        }

        tcp::socket &socket() {
            return socket_;
        }

        void start(std::string response) {
            message_ = response;
            boost::asio::async_write(socket_, boost::asio::buffer(message_),
                                        boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
        }

        private:
        tcp_connection(boost::asio::io_service& io_service) : socket_(io_service) {}
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred) {}
        tcp::socket socket_;
        std::string message_;
};

