#include <iostream>
#include <boost/unordered_map.hpp>
#include <mutex>

template <class V> class HashMap {

    public:
    HashMap() {}

    V get(int key) { 
        if (lock.try_lock()) {
            // lock.lock();
            V val = map[key];
            // lock.unlock();
            return val;
        }
        return NULL;
    }
    
    bool put(int key, V value) {
        if (lock.try_lock()) {
            // lock.lock();
            map[key] = value;
            // lock.unlock();
            return true;
        }
        return false;
     }

    private:
    boost::unordered_map<int, V> map;
    std::mutex lock;
};