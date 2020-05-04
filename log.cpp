#include <iostream>
#include <thread>
#include <fstream>
#include <mutex>
#include <string>

const std::string node_log_filename = "node_log.txt";
const std::string app_log_filename = "app_log.txt";

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
    // std::cerr << sstr.str();
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

void app_log(std::string msg) {
    return;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    static std::ofstream out(app_log_filename, std::ofstream::app);
    out << std::this_thread::get_id() << ": " << msg << std::endl;
    out.flush();
} 

void node_log(std::string msg) {
    return;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    static std::ofstream out(node_log_filename, std::ofstream::app);
    out << std::this_thread::get_id() << ": " << msg << std::endl;
    out.flush();
} 