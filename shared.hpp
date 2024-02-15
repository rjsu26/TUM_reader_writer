#include <cstdint>
#include <string>
#include <semaphore.h>

enum task_state{
    FAILURE = -1,
    SUCCESS,
    NOT_PROCESSED,
    PROCESSING,
};

const int BUFFER_SIZE = 10; 

struct SharedNode {
    char message[20]; // insert, read, delete or error message 
    int insert_key = INT32_MIN;
    int insert_val= INT32_MIN; 
    int read_key= INT32_MIN; 
    int read_val= INT32_MIN; 
    int delete_key= INT32_MIN; 
    task_state processed = SUCCESS;  // client needs to set this to not_processed for server to process this

    // TODO : use union to save space as at a given time
    // only one operation and its related fields will be accessed
};

struct SharedData{
    SharedNode buffer[BUFFER_SIZE];
    int read_idx=0; // points to node available to read
    int write_idx=0; // points to available slot to write on
    // sem_t mutex;
    sem_t filled_slots; 
    sem_t empty_slots; 
};