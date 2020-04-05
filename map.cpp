#include <iostream>
#include <boost/unordered_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <mutex>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/types.h> 

//TODO change to hash on something other than direct key value, to avoid skew
template <class V> class HashMap {

    const int NUM_LOCKS = 100000;
    public:
    HashMap() {
        for (int i = 0; i < NUM_LOCKS; i++) {
            lock_list.push_back(new std::mutex());
        }
    }

    ~HashMap() {
        lock_list.clear();
    }

    V get(int key) { 
        // if (lock.try_lock()) {
        V val = map[key];
            // lock.unlock();
        return val;
        // }
        // return NULL;
    }
    
    void put(int key, V value) {
        // if (lock.try_lock()) {
        map[key] = value;
        // lock.unlock();
        // return true;
        // }
        // return false;
     }

     bool try_lock(int key) {
         if (lock_list[key % lock_list.size()].try_lock()) {
             return true;
         }
         return false;
     }

    void unlock(int key) {
        lock_list[key % lock_list.size()].unlock(); 
    }

    private:
    boost::unordered_map<int, V> map;
    boost::ptr_vector<std::mutex> lock_list;
    // std::mutex lock;
};