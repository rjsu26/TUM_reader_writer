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
#include <cstdlib> // rand()
#include <ctime>
#include <csignal>

#include "shared.hpp"

using namespace std;

bool prog_terminate = false;

void signalHandler(int signal)
{
    cout << "Terminating.." << endl;
    prog_terminate = true;
}

int main()
{
    // Register the signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    // Open the POSIX shared memory object
    int shm_fd = shm_open("/shared_memory", O_RDWR, 0666);
    if (shm_fd == -1)
    {
        cerr << "Error opening shared memory segment" << endl;
        return -2;
    }

    // Map the shared memory segment into the address space of the process
    SharedData *shared_data = (SharedData *)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED)
    {
        cerr << "Error mapping shared memory" << endl;
        return -3;
    }
    string ops[] = {"insert", "read", "delete"};
    srand(time(0)); // seed the rng

    // Read data from the shared memory segment
    while (!prog_terminate)
    {
        sem_wait(&shared_data->empty_slots);
        sem_wait(&shared_data->mutex);

        // generating random ops
        int ops_idx = rand() % 3;
        SharedNode *data = &shared_data->buffer[shared_data->write_idx];
        shared_data->write_idx = (shared_data->write_idx + 1) % BUFFER_SIZE;
        strncpy(data->message, ops[ops_idx].c_str(), 20);

        // prepare packet
        if (ops[ops_idx] == "insert")
        {
            // initialize the insert keys and values
            data->insert_key = rand() % INT16_MAX;
            data->insert_val = rand() % INT16_MAX;
        }
        else if (ops[ops_idx] == "read")
        {
            data->read_key = rand() % INT16_MAX;
        }
        else
        {
            data->delete_key = rand() % INT16_MAX;
        }
        cout << "Submitted " << ops[ops_idx]<<" idx : "<< (shared_data->write_idx - 1) % BUFFER_SIZE<< endl;
        sem_post(&shared_data->mutex);
        sem_post(&shared_data->filled_slots);

        // usleep(200000);
    }

    // Unmap and close the shared memory segment
    munmap(shared_data, sizeof(SharedData));
    shm_unlink("/shared_memory");
    close(shm_fd);
    cout<<"All resources cleaned up!"<<endl;

    return 0;
}
