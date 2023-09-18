## OS -- Assignment_1
##### Author: Yannis Zervakis

### *General*
In  this assignment we are trying to simulate and synchronize a readers-writers problem. In fact, we have one main writer (parent process) who satisfies requests from many readers (children).

### *Semaphores*
In order to synchronize requests and answers we are using a batch of semaphores : we have one semaphore for each segment plus four more semaphores. We need a semaphore for each segment so as to synchronize all different child-processes that want the same segment of text and don't have mutual exclusion between them. We also need four more semaphores : SYNC_REQUEST (parent waits on it until a child has a request), SYNC_ANSWER (children wait on it until parent has answered the request), SYNC_PARENT_ACCESS (parent wait on it until a batch of processes that want the same segment have end take it and and parent can write a new segment in shared memory segment), SYNC_CHILD_ACCESS (children processes wait on it in first time that a segment is wanted). The thing is that the semaphore for current segment is up when is the time to *read* and this is the way to not have mutual exclusion.

### Shared Memory Segment
The structure of shared memory segment that is using this program is described in *common.h* file. Is important to notice that we are handling the shared memory segment as a batch of bytes, and in all processes we are creating a stack object type of *SharedData* to keep the information for more clearness. Shared memory segment has pointers to two unsigned integers (current transaction, used for check if all transactions are done and end the parent's loop and no_requested_segment that holds the child's request). Also we have a double pointer to char which is the requested segment, an dynamic array of semaphores and a dynamic array of unsigned integers called readers(one reader for each segment, holds the number of child-processes that are reading a specific segment).

### Extra
We use some extra deallocation routines that cleans the memory that has been allocated for a purpose, a routine report_and_exit that reports a specific error from error codes that are defined in *common.h* as a enum.

### Usage

In order to compile and produce the executables programs type :
-  make

In order to run a test for function get_segment type :
- `make run-tests`

In order to run the program type :
- `make run text=*filename.txt* children=*no_children* transactions=*no_transactions* lines_per_segment=*no_lines_per_segment*`

Example : 
`make run text=sample.txt children=10 transactions=100 lines_per_segment=150`

Results of form *ChildXXXX.txt* are in results directory.
After all you can type :
- `make clean`

in order to clean up everything.

 
