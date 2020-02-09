#include <iostream>
#include <boost/unordered_map.hpp>
#include <mutex>

template <class V> class HashMap {

    public:
    HashMap() {}

    V get(int key) { 
        int l = std::trylock(lock);
        if (l == -1) {
            V val = map[key];
            lock.unlock();
            return val;
        }
        return NULL;
    }
    
    bool put(int key, V value) {
        int l = std::trylock(lock);
        if (l == -1) {
            map[key] = value;
            lock.unlock();
            return true;
        }
        return false;
     }

    //Does this key belong to this node?
    // bool key_in_range(int key) { return false; }

    private:
    boost::unordered_map<int, V> map;
    std::mutex lock;
};