#include "server.hpp"


using namespace std;

/* Function called in a new thread to execute db operations like insert, find
  @input : void pointer to database object
  @return : NULL
 */
void *database_operations(void *arg)
{
    ThreadArgs *args = reinterpret_cast<ThreadArgs*>(arg);
    Map *db = args->db;
    SharedNode *data = args->data;

    if (strncmp(data->message, "insert", 20) == 0)
    {
        if (data->insert_key == INT32_MIN || data->insert_val == INT32_MIN)
        {
            data->processed = FAILURE; // this ops failed
            // set error message
            strncpy(data->message, "Key|Value not given", 20);

            // TODO : log the invalid operation somewhere
        }
        else
        {
            bool status = db->insert(data->insert_key, data->insert_val);
            if (!status)
            {
                data->processed = FAILURE;
                // set error message
                strncpy(data->message, "Key already present", 20);
            }
            else
            {
                data->processed = SUCCESS;
                cout << "Processed insert" << endl;
            }
        }

        // reset key and value to INT_MIN for preventing garbage read
        data->insert_key = INT32_MIN;
        data->insert_val = INT32_MIN;
    }
    else if (strncmp(data->message, "read", 20) == 0)
    {
        if (data->read_key == INT32_MIN)
        {
            data->processed = FAILURE; // this ops failed
            // set error message
            strncpy(data->message, "Read key not given", 20);
            // TODO : log the invalid operation somewhere
        }
        else
        {
            bool status = db->find(data->read_key, data->read_val);
            if (!status)
            {
                data->processed = FAILURE;
                // set error message
                strncpy(data->message, "No such key found", 20);
            }
            else
            {
                data->processed = SUCCESS;
                cout << "Processed find" << endl;
            }
        }

        // reset key to INT_MIN for preventing garbage read
        data->read_key = INT32_MIN;
    }
    else if (strncmp(data->message, "delete", 20) == 0)
    {
        if (data->delete_key == INT32_MIN)
        {
            data->processed = FAILURE; // this ops failed
            // set error message
            strncpy(data->message, "key not given", 20);
            // TODO : log the invalid operation somewhere
        }
        else
        {
            bool status = db->remove(data->delete_key);
            if (!status)
            {
                data->processed = FAILURE;
                // set error message
                strncpy(data->message, "No such key found", 20);
            }
            else
            {
                data->processed = SUCCESS;
                cout << "Processed delete" << endl;
            }
        }
        // reset key to INT_MIN for preventing garbage read
        data->delete_key = INT32_MIN;
    }
    else // unrecognized command
    {
        strncpy(data->message, "Unrecognized !", 20);
        data->processed = FAILURE;
    }

    return NULL; 
}

int main(int argc, char *argv[])
{
    cout << "Starting the server...." << endl;

    // Create a POSIX shared memory object
    int shm_fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        cerr << "Error creating shared memory segment" << endl;
        return -2;
    }

    // Configure the size of the shared memory segment
    ftruncate(shm_fd, sizeof(SharedData));

    // Map the shared memory segment into the address space of the process
    SharedData *shared_data = (SharedData *)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED)
    {
        cerr << "Error mapping shared memory" << endl;
        return -3;
    }

    /* Initialize the semaphores */
    assert(sem_init(&shared_data->mutex, 1, 1) == 0);
    assert(sem_init(&shared_data->empty_slots, 1, BUFFER_SIZE) == 0);
    assert(sem_init(&shared_data->filled_slots, 1, 0) == 0);

    /* Initialize the hash table
     * read commandline argument for number of buckets in the hash table specified as "./server --table-size 10"
     * If no values, default to 5
     */

    int bucket_size = 5;

    if (argc > 2 && strncmp(argv[1], "--table-size", 12) == 0)
        bucket_size = atoi(argv[2]); // TODO : handle exception when --table-size is not a number

    cout << "Initializing hash table with " << bucket_size << " buckets..." << endl;

    Map db(bucket_size);

    // Write data to the shared memory segment

    cout << "Taking requests from client...." << endl;

    while (true)
    {
        sem_wait(&shared_data->filled_slots);
        sem_wait(&shared_data->mutex); // Wait for access to shared memory

        SharedNode *data = &shared_data->buffer[shared_data->read_idx];
        shared_data->read_idx = (shared_data->read_idx + 1) % BUFFER_SIZE;
        cout << "Executing " << data->message << " idx : " << (shared_data->read_idx - 1) % BUFFER_SIZE << endl;
        if (data->processed == NOT_PROCESSED) // state changed by either of the if conditions below
        {
            ThreadArgs args{
                .db = &db,
                .data = data};

            // Create thread
            pthread_t thread;
            if (pthread_create(&thread, nullptr, database_operations, &args) != 0)
            {
                std::cerr << "Error creating thread" << std::endl;
                return 1;
            }

            // Wait for thread to finish
            if (pthread_join(thread, nullptr) != 0)
            {
                std::cerr << "Error joining thread" << std::endl;
                return 1;
            }
        }
        sem_post(&shared_data->mutex); // Release access to shared memory
        sem_post(&shared_data->empty_slots);

        // sleep(1);
    }

    sem_destroy(&shared_data->mutex);
    sem_destroy(&shared_data->empty_slots);
    sem_destroy(&shared_data->filled_slots);

    // Unmap and close the shared memory segment
    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);

    return 0;
}
