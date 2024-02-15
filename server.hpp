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
#include <cmath>

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
        return (key + key % buckets_size)% buckets_size;
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
        cout << "\t[db] Inserting " << key << " into bucket:" << hash(key) << endl;
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
                cout << "\t[db] Found " << key << " in bucket:" << hash(key) << endl;
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
                cout << "\t[db] Deleting " << key << " from bucket:" << hash(key) << endl;
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
    // SharedNode *node;
    SharedData *shared_data;
};