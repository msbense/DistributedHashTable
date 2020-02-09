#include <iostream>
#include <boost/unordered_map.hpp>

template <class V> class HashMap {

    public:
    HashMap() {}

    V get(int key) { 
        return map[key];
    }
    
    bool put(int key, V value) {
        map[key] = value;
        return true;
     }

    //Does this key belong to this node?
    // bool key_in_range(int key) { return false; }

    private:
    boost::unordered_map<int, V> map;
};