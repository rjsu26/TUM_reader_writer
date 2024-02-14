Readers Writers program
-----------------------------
Two programs communicating using shared memory (POSIX shm_* APIs). The shared memory is in the form of a circular ring buffer with a mutex to allow only a reader or a writer at any given instant. Additionally, two semaphores _filled\_slots_ and _empty\_slots_ are used to signal if a consumer or a producer should proceed, respectively. 

# Source files

## server.cpp and server.hpp
The code corresponding to reader. For each item in the buffer, creates a new thread and delegates the database operation to it. Since order of db operations were assumed to be important, true parallelism was not aimed and the threads are being waited for before moving on to next element in the buffer. 

## client.cpp 
The producer code which randomly generates db operations (insert, read or delete) and puts the request into the buffer. 
Currently, the code does not check for results however the server is always updating the corresponding memory with results (say success or failure for a delete) which can be checked in the client code in later versions. 

To support safe termination, client code also handles SIGINT (Ctrl + C) which makes the program de-init all the file descriptors and shared memory before exiting. 

Due to concurrency safety, multiple clients can operate on a particular server instance. 

## shared.hpp
Contains definition of the shared buffer and definition of each node in the buffer. Used by producer and consumer to create and process commands, respectively. 

-------------------------------------------------

# Instruction 
To run, do : 
```
make clean
make
```
to generate the server and client executable binaries. 

In two different shell, run `./server --table-size 10` and `./client` in that order. 

Note : the table-size option is to configure number of buckets in the hash table and if not provided will default to 5. 