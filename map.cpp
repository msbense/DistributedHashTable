#include <iostream>
#include <boost/unordered_map.hpp>
#include <mutex>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/types.h> 

template <class V> class HashMap {

    public:
    HashMap() {}

    V get(int key) { 
        // std::cerr << "PID: " << getpid() << " getting " << key << std::endl;
        if (lock.try_lock()) {
            // lock.lock();
            // std::this_thread::sleep_for (std::chrono::seconds(1));
            V val = map[key];
            lock.unlock();
            // std::cerr << "PID: " << getpid() << " successfully got " << key << " " << val << std::endl;
            return val;
        }
        // std::cerr << "PID: " << getpid() << " couldn't get value for " << key << std::endl;
        return NULL;
    }
    
    bool put(int key, V value) {
        // std::cerr << "PID: " << getpid() << " putting " << key << " " << value << std::endl;
        if (lock.try_lock()) {
            // std::this_thread::sleep_for (std::chrono::seconds(1));
            // lock.lock();
            map[key] = value;
            lock.unlock();
            // std::cerr << "PID: " << getpid() << " successfuly put " << key << " " << value << std::endl;
            return true;
        }
        // std::cerr << "PID: " << getpid() << " couldn't put value for " << key << std::endl;
        return false;
     }

    private:
    boost::unordered_map<int, V> map;
    std::mutex lock;
};