#include <iostream>
#include <thread>


void print(std::string msg) {
    std::stringstream sstr;
    sstr << msg << std::endl;
    // std::cerr << sstr.str();
}

void thread_print(std::string msg) {
    std::stringstream sstr;
    sstr << "Thread " << std::this_thread::get_id() << ": " << msg << std::endl;
    // std::cerr << sstr.str();
}