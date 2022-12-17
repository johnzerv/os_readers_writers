#include <sys/wait.h>

#include "common.h"

// TODO :  README
//        cleanup - check for results

int main(int argc, char **argv) {
    if (argc != NO_ARGUMENTS) {
        fprintf(stderr, "Wrong Arguments\n");
        return -1;
    }

    srand(time(NULL)); // Using rand() in child processes

    // Arguments
    char *filename = malloc(sizeof(char)*strlen(argv[1])+1);
    if (filename == NULL) {
        report_and_exit("malloc", MALLOC_ERROR);
    }
    else
        strcpy(filename, argv[1]);

    unsigned no_children = atoi(argv[2]), no_transactions = atoi(argv[3]);
    unsigned lines_per_segment = atoi(argv[4]);

    // Get input's total lines
    unsigned input_lines = get_input_lines(filename);
    if (input_lines == -1) {
        free(filename);
        report_and_exit("fopen", FOPEN_ERROR);
    } else if (input_lines < 1000) {
        fprintf(stderr, "Input file with #lines < 1000\n");
        return -1;
    }

    unsigned int lines_div = input_lines / lines_per_segment;
    unsigned int no_segments = (input_lines % lines_per_segment == 0) ? lines_div : lines_div + 1;

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        free(filename);
        report_and_exit("fopen", FOPEN_ERROR);
    }

    char **segment;
    if ((segment = malloc(sizeof(char*) * lines_per_segment)) == NULL) {
        free(filename);
        report_and_exit("malloc", MALLOC_ERROR);
    }
    for (int i = 0; i < lines_per_segment; i++) {
        if ((segment[i] = malloc(sizeof(char) * MAX_LINE_SIZE)) == NULL) {
            for (int j = i - 1; j >= 0; j--) {
                free(segment[j]);
            }
            free(filename);
            report_and_exit("malloc", MALLOC_ERROR);
        }
    }

    //Shared memory info
    int  shmid = -1;    // Initialize shmid before shmget
    void *shm = NULL; // Initialize shared segment pointer before shmat
    SharedData shared;

    // Fork-Wait variables
    pid_t pid;
    int status;

    // Total size of shared memory segment
    unsigned total_size = 2*sizeof(unsigned int) + lines_per_segment * (MAX_LINE_SIZE + 1) * sizeof(char) + no_segments * sizeof(unsigned int) + (no_segments + EXTRA_SEMAPHORES) * sizeof(sem_t);

    // Create shared memory segment
    if ((shmid = shmget(IPC_PRIVATE, total_size, 0600 | IPC_CREAT)) < 0) {
        free(filename);
        free_segment(segment, lines_per_segment);
        report_and_exit("shmget", SHMGET_ERROR);
    }

    // Attach the shared memory segment
    if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
        free(filename);
        free_segment(segment, lines_per_segment);
        report_and_exit("shmat", SHMAT_ERROR);
    }

    // Keep shared memory segment fields in SharedData object for convenience
    shared.current_transaction = (unsigned int *)shm;
    shared.no_requested_segment = (unsigned int*)(shm + sizeof(unsigned int));
    shared.readers = (unsigned int*)(shm + 2*sizeof(unsigned int) + lines_per_segment * MAX_LINE_SIZE * sizeof(char));
    shared.semaphores = (sem_t*)(shm + 2*sizeof(unsigned int) + lines_per_segment * MAX_LINE_SIZE * sizeof(char) + no_segments * sizeof(unsigned int));
    shared.requested_segment = malloc(lines_per_segment * sizeof(char*));
    if (shared.requested_segment == NULL) {
        free(filename);
        free_segment(segment, lines_per_segment);
        free_shm(shmid, shm);
        report_and_exit("malloc", MALLOC_ERROR);
    }
    for (int i = 0; i < lines_per_segment; i++) {
        shared.requested_segment[i] = (char *)(shm + 2*sizeof(int) + i * (MAX_LINE_SIZE + 1));
    }

    // Create semaphores and initialize them
    if (sem_init(&(shared.semaphores[SYNC_REQUEST]), 1, 0)) {
        free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, 0);
        report_and_exit("sem_init", SEM_INIT_ERROR);
    }

    if (sem_init(&(shared.semaphores[SYNC_ANSWER]), 1, 0)) {
        free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, SYNC_ANSWER);       
        report_and_exit("sem_init", SEM_INIT_ERROR);
    }

    if (sem_init(&(shared.semaphores[SYNC_PARENT_ACCESS]), 1, 0)) {
        free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, SYNC_PARENT_ACCESS);      
        report_and_exit("sem_init", SEM_INIT_ERROR);
    }

    if (sem_init(&(shared.semaphores[SYNC_CHILD_ACCESS]), 1, 0)) {
        free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, SYNC_CHILD_ACCESS);
        report_and_exit("sem_init", SEM_INIT_ERROR);
    }

    for (unsigned i = EXTRA_SEMAPHORES; i < no_segments + EXTRA_SEMAPHORES; i++) {
        if (sem_init(&shared.semaphores[i], 1, 1)) {
            free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, i);
            report_and_exit("sem_init", SEM_INIT_ERROR);
        }
    }

    // Initialize readers and current transaction
    for (unsigned i = 0; i < no_segments; i++) {
        shared.readers[i] = 0;
    }
    *(shared.current_transaction) = 0;

    // Create children
    for (int i = 0; i < no_children; i++) {
        pid = fork();

        if (pid == -1){  // Error
            free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, no_segments + EXTRA_SEMAPHORES);
            report_and_exit("fork", FORK_ERROR);
        }
        // And run their code
        else if (pid == 0) {   // Child
            // Prepare arguments for child
            char lines[10], shmid_string[10], segments[10], lines_per_seg[10];
            sprintf(lines, "%d", input_lines);
            sprintf(shmid_string, "%d", shmid);
            sprintf(segments, "%d", no_segments);
            sprintf(lines_per_seg, "%d", lines_per_segment);

            execl("./child", lines, argv[3], shmid_string, segments, lines_per_seg, NULL);
        }
    }

    // Satisfy all requests
    while(*(shared.current_transaction) != no_children * no_transactions) {
        if (sem_post(&(shared.semaphores[SYNC_CHILD_ACCESS]))) {
            free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, no_segments + EXTRA_SEMAPHORES);
            report_and_exit("sem_post", SEM_POST_ERROR);
        }


        if (sem_wait(&(shared.semaphores[SYNC_REQUEST]))) {  // Waiting for a request
            free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, no_segments + EXTRA_SEMAPHORES);
            report_and_exit("sem_wait", SEM_WAIT_ERROR);
        }

        // write requested segment
        get_segment(fp, *(shared.no_requested_segment), shared.requested_segment, no_segments, lines_per_segment, input_lines);

        if (sem_post(&(shared.semaphores[SYNC_ANSWER]))) {  // Satisfaction of request
            free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, no_segments + EXTRA_SEMAPHORES);
            report_and_exit("sem_post", SEM_POST_ERROR);
        }

        
        if (sem_wait(&(shared.semaphores[SYNC_PARENT_ACCESS]))) { 
            free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, no_segments + EXTRA_SEMAPHORES);
            report_and_exit("sem_wait", SEM_POST_ERROR);
        }
    }

    // Collect child's returned values
    for (int i = 0; i < no_children; i++) {
        printf("\nStatus from child with PID %d collected", wait(&status));
    }
    putchar('\n');  // Pretty print

    free_resources(filename, segment, lines_per_segment, shared.requested_segment, shmid, shm, shared.semaphores, no_segments + EXTRA_SEMAPHORES);
    fclose(fp);

    return 0;
}