#include <iostream>
#include <boost/unordered_map.hpp>
#include <mutex>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/types.h> 

//TODO change to hash on something other than direct key value, to avoid skew
template <class V> class HashMap {

    public:
    HashMap() {
        
    }

    V get(int key) { 
        if (lock.try_lock()) {
            V val = map[key];
            lock.unlock();
            return val;
        }
        return NULL;
    }
    
    bool put(int key, V value) {
        if (lock.try_lock()) {
            map[key] = value;
            lock.unlock();
            return true;
        }
        return false;
     }

     bool try_lock(int key) {
         if (lock.try_lock()) {
             return true;
         }
         return false;
     }

     void unlock(int key) {
         lock.unlock();
     }

    private:
    boost::unordered_map<int, V> map;
    std::mutex lock;
};