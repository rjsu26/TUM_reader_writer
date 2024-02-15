#include <csignal>
#include "server.hpp"

using namespace std;

bool prog_terminate = false;

void signalHandler(int signal)
{
    cout << "Terminating.." << endl;
    exit(-5);
}

/* Function called in a new thread to execute db operations like insert, find
  @input : void pointer to database object
  @return : NULL
 */
void *database_operations(void *arg)
{
    ThreadArgs *args = reinterpret_cast<ThreadArgs *>(arg);
    Map *db = args->db;
    SharedData *shared_data = args->shared_data;
    SharedNode *node = &shared_data->buffer[shared_data->read_idx % BUFFER_SIZE];
    int current_read_idx = shared_data->read_idx; 
    shared_data->read_idx = (shared_data->read_idx + 1) ;
    // cout<<__LINE__<<endl;
    if (node->processed == NOT_PROCESSED) // state changed by either of the if conditions below
    {
        // cout<<__LINE__<<endl;
        node->processed = PROCESSING; 

        if (strncmp(node->message, "insert", 20) == 0)
        {
            if (node->insert_key == INT32_MIN || node->insert_val == INT32_MIN)
            {
                node->processed = FAILURE; // this ops failed
                // set error message
                strncpy(node->message, "Key|Value not given", 20);
                cout<<"\tKey|Value not given"<<endl;
                // TODO : log the invalid operation somewhere
            }
            else
            {
                cout << "Executing insert "
                     << " idx : " << current_read_idx
                     << " key : " << node->insert_key << endl;
                bool status = db->insert(node->insert_key, node->insert_val);
                if (!status)
                {
                    node->processed = FAILURE;
                    // set error message
                    strncpy(node->message, "Key already present", 20);
                      cout<<"\tKey already present"<<endl;
              
                }
                else
                {
                    node->processed = SUCCESS;
                }
            }

            // reset key and value to INT_MIN for preventing garbage read
            node->insert_key = INT32_MIN;
            node->insert_val = INT32_MIN;
        }
        else if (strncmp(node->message, "read", 20) == 0)
        {
            if (node->read_key == INT32_MIN)
            {
                node->processed = FAILURE; // this ops failed
                // set error message
                strncpy(node->message, "Read key not given", 20);
                cout<<"\tRead key not given"<<endl;
                // TODO : log the invalid operation somewhere
            }
            else
            {
                cout << "Executing read "
                     << " idx : " << current_read_idx
                     << " key : " << node->read_key << endl;

                bool status = db->find(node->read_key, node->read_val);
                if (!status)
                {
                    node->processed = FAILURE;
                    // set error message
                    strncpy(node->message, "No such key found", 20);
                    cout<<"\tNo such key found"<<endl;
                
                }
                else
                {
                    node->processed = SUCCESS;
                }
            }

            // reset key to INT_MIN for preventing garbage read
            node->read_key = INT32_MIN;
        }
        else if (strncmp(node->message, "delete", 20) == 0)
        {
            if (node->delete_key == INT32_MIN)
            {
                node->processed = FAILURE; // this ops failed
                // set error message
                strncpy(node->message, "key not given", 20);
                cout<<"\tkey not given"<<endl;

                // TODO : log the invalid operation somewhere
            }
            else
            {
                cout << "Executing remove "
                     << " idx : " << current_read_idx
                     << " key : " << node->delete_key << endl;

                bool status = db->remove(node->delete_key);
                if (!status)
                {
                    node->processed = FAILURE;
                    // set error message
                    strncpy(node->message, "No such key found", 20);
                     cout<<"\tNo such key found"<<endl;
                
                }
                else
                {
                    node->processed = SUCCESS;
                }
            }
            // reset key to INT_MIN for preventing garbage read
            node->delete_key = INT32_MIN;
        }
        else // unrecognized command
        {
            strncpy(node->message, "Unrecognized !", 20);
            node->processed = FAILURE;
        }
    }
    // cout<<__LINE__<<endl;

    sem_post(&shared_data->empty_slots);
    return NULL;
}

int main(int argc, char *argv[])
{
    // Register the signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    cout << "Starting the server...." << endl;

    // Create a POSIX shared memory object
    int shm_fd = shm_open("/shared_memory", O_CREAT | O_RDWR | O_TRUNC, 0666);
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
    // assert(sem_init(&shared_data->mutex, 1, 1) == 0);
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
    cout << "-------------------------------"<<endl;

    while (true)
    {
        sem_wait(&shared_data->filled_slots);
        // sem_wait(&shared_data->mutex); // Wait for access to shared memory
        ThreadArgs args{
            .db = &db,
            // .node = node,
            .shared_data = shared_data};

        // Create thread
        pthread_t thread;
        if (pthread_create(&thread, nullptr, database_operations, &args) != 0)
        {
            std::cerr << "Error creating thread" << std::endl;
            return -4;
        }

        // Detach thread1
        if (pthread_detach(thread) != 0)
        {
            std::cerr << "Error detaching thread" << std::endl;
            return -5;
        }
        // sem_post(&shared_data->mutex); // Release access to shared memory

        // sleep(1);
    }

    // sem_destroy(&shared_data->mutex);
    sem_destroy(&shared_data->empty_slots);
    sem_destroy(&shared_data->filled_slots);

    // Unmap and close the shared memory segment
    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);

    return 0;
}
