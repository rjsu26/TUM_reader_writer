#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <mutex>
#include <vector>
#include <list>
#include <functional>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <cassert>
#include <pthread.h>

#include "shared.hpp"

using namespace std;

class Map
{
private:
    int buckets_size;
    vector<list<pair<int, int>>> buckets_list;
    vector<mutex> locks;

    int hash(int key)
    {
        // TODO : improve the hash function
        return key % buckets_size;
    }

public:
    Map(int buckets)
    {
        this->buckets_size = buckets;
        this->buckets_list.resize(buckets);
        // this->locks.resize(buckets);
        vector<mutex> locks(buckets);
        this->locks.swap(locks);

    }

    bool insert(int key, int value)
    {
        lock_guard<mutex> lock(locks[hash(key)]);
        auto &bucket = buckets_list[hash(key)];
        for (auto &pair : bucket)
        {
            if (pair.first == key)
            {
                pair.second = value;
                return false;
            }
        }
        bucket.push_back({key, value});
        return true;
    }

    bool find(int key, int &value)
    {
        lock_guard<mutex> lock(locks[hash(key)]);
        auto &bucket = buckets_list[hash(key)];
        for (const auto &pair : bucket)
        {
            if (pair.first == key)
            {
                value = pair.second;
                return true;
            }
        }
        return false;
    }

    bool remove(int key)
    {
        lock_guard<mutex> lock(locks[hash(key)]);
        auto &bucket = buckets_list[hash(key)];
        auto it = bucket.begin();
        while (it != bucket.end())
        {
            if (it->first == key)
            {
                bucket.erase(it);
                return true;
            }
            ++it;
        }
        return false;
    }
};


struct ThreadArgs{
    Map *db;
    SharedNode *data;
};