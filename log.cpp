#include <iostream>
#include <thread>
#include <fstream>
#include <mutex>
#include <string>

const std::string log_filename = "log.txt";

void print(std::string msg) {
    std::stringstream sstr;
    sstr << msg << std::endl;
    std::cerr << sstr.str();
}

void thread_print(std::string msg) {
    // static std::mutex m;
    // std::lock_guard<std::mutex> lock(m);
    std::stringstream sstr;
    sstr << "Thread " << std::this_thread::get_id() << ": " << msg << std::endl;
    std::cerr << sstr.str();
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

void log(std::string msg) {
    return;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    static int lognum = std::rand() % 100; 
    static std::ofstream out(log_filename + std::to_string(lognum), std::ofstream::app);
    out << msg << std::endl;
    out.flush();
} 
