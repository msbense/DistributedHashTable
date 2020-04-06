#include <iostream>
#include <thread>
#include <fstream>
#include <mutex>


void print(std::string msg) {
    std::stringstream sstr;
    sstr << msg << std::endl;
    std::cerr << sstr.str();
}

void thread_print(std::string msg) {
    // static std::mutex m;
    // std::lock_guard<std::mutex> lock(m);
    // std::stringstream sstr;
    // sstr << "Thread " << std::this_thread::get_id() << ": " << msg << std::endl;
    // // std::cerr << sstr.str();
    // std::ofstream out;
    // static bool append = false;
    // if (!append)
    //     out.open("applog.txt", std::ios_base::app);
    // else {
    //     out.open("applog.txt");
    //     append = true;
    // }
    // out << sstr.str();
    // out.close();

}