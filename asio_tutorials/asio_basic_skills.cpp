#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


class printer {
    private:
    int count_;
    boost::asio::deadline_timer timer1_;
    boost::asio::deadline_timer timer2_;
    boost::asio::io_service::strand strand_;
    public:
    
    printer(boost::asio::io_service& io) :  strand_(io),
                                            timer1_(io, boost::posix_time::seconds(1)),
                                            timer2_(io, boost::posix_time::seconds(1)), 
                                            count_(0) 
    {
        timer1_.async_wait(strand_.wrap(boost::bind(&printer::print1, this)));
        timer2_.async_wait(strand_.wrap(boost::bind(&printer::print2, this)));
    } 
    
    void print1() {
        if (count_ < 10) {
            count_++;
            std::cout << "T1 " << count_ << std::endl;
            timer1_.expires_at(timer1_.expires_at() + boost::posix_time::seconds(1));
            timer1_.async_wait(strand_.wrap(boost::bind(&printer::print1, this)));
        }
    }

    void print2() {
        if (count_ < 10) {
            count_++;
            std::cout << "T2 " << count_ << std::endl;
            timer2_.expires_at(timer2_.expires_at() + boost::posix_time::seconds(1));
            timer2_.async_wait(strand_.wrap(boost::bind(&printer::print2, this)));
        }
    }

    ~printer() {
        std::cout << "End " << count_ << std::endl;
    }


};

// void async_print(const boost::system::error_code& e, boost::asio::deadline_timer *t, int* count) {
//     if (*count < 5) {
//         std::cout << *count << std::endl;
//         (*count)++;
//         t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
//         t->async_wait(boost::bind(async_print, boost::asio::placeholders::error, t, count));
//     }
// }

int main() {
    std::cout << "Start" << std::endl;

    boost::asio::io_service io;
    printer p(io);
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io));
    // int count = 0;
    // boost::asio::deadline_timer t(io, boost::posix_time::seconds(1));

    // t.async_wait(boost::bind(async_print, boost::asio::placeholders::error, &t, &count));
    io.run();
    t.join();
    // std::cout << "End " << count << std::endl;
}