#include "common.h"

int main(int argc, char **argv) {
    unsigned int transactions = atoi(argv[1]);
    int shmid = atoi(argv[2]);
    unsigned int no_segments = atoi(argv[3]);
    unsigned int lines_per_segment = atoi(argv[4]);
    unsigned int requested_segment = 0;
    char requested_line[MAX_LINE_SIZE];
    unsigned int no_requested_line = 0;

    struct timeval timestamp_request, timestampt_answer;
    struct tm *tm_request, *tm_answer;

    // Create a string as "child_XXXXX", where XXXXX the child's pid
    char filename[20] = "child_"; // Assuming pid is at most 14 digits
    char child_pid[14];
    sprintf(child_pid, "%d", getpid());
    strcat(filename, child_pid);
    strcat(filename, ".txt");

    FILE *fp = fopen(filename, "w");    // Create a file to store child_XXXXX's info about requests
    if (fp == NULL) {
        report_and_exit("fopen", FOPEN_ERROR);
    }

    void *shm;
    SharedData shared;

    if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
        report_and_exit("shmat", SHMAT_ERROR);
    }

    shared.current_transaction = (unsigned int *)shm;
    shared.no_requested_segment = (unsigned int*)(shm + sizeof(unsigned int));

    shared.requested_segment = malloc(lines_per_segment * sizeof(char*));
    if (shared.requested_segment == NULL) {
        report_and_exit("malloc", MALLOC_ERROR);
    }
    for (int i = 0; i < lines_per_segment; i++) {
        shared.requested_segment[i] = (char *)(shm + 2*sizeof(int) + i * (MAX_LINE_SIZE + 1));
    }
    shared.readers = (unsigned int*)(shm + 2*sizeof(unsigned int) + lines_per_segment * MAX_LINE_SIZE * sizeof(char));
    shared.semaphores = (sem_t*)(shm + 2*sizeof(unsigned int) + lines_per_segment * MAX_LINE_SIZE * sizeof(char) + no_segments * sizeof(unsigned int));

    // // Transactions (Requests - Answers)
    for (unsigned int i = 0; i < transactions; i++) {
        // Pick the same segment with propability 70%
        if (i == 0) {
            requested_segment = rand() % no_segments;
        } else {
            unsigned int propability = rand() % 100;
            requested_segment = (propability < 70) ? rand() % no_segments : requested_segment;
        }
        
        do {
            no_requested_line = rand() % lines_per_segment;
        } while (no_requested_line + lines_per_segment*requested_segment >= lines_per_segment*no_segments);

        if (sem_wait(&(shared.semaphores[requested_segment+EXTRA_SEMAPHORES]))) {
            report_and_exit("sem_wait", SEM_WAIT_ERROR);
        }

        // Keep timestamp of request
        gettimeofday(&timestamp_request, NULL);
        tm_request = localtime(&timestamp_request.tv_sec);
        
        // If this is the first reader for current <requested_segment>
        if (++(shared.readers[requested_segment]) == 1) {
            if (sem_wait(&(shared.semaphores[SYNC_CHILD_ACCESS]))) {
                report_and_exit("sem_wait", SEM_WAIT_ERROR);
            }

            // Update requested segment in shared memory segment
            *(shared.no_requested_segment) = requested_segment;

            // Request to parent-process
            if (sem_post(&(shared.semaphores[SYNC_REQUEST]))) {
                report_and_exit("sem_post", SEM_POST_ERROR);
            }

            // Wait for the answer
            if (sem_wait(&(shared.semaphores[SYNC_ANSWER]))) {
                report_and_exit("sem_wait", SEM_WAIT_ERROR);
            }
        }

        // We don't have mutual exclusive between processes that reading same segment
        if (sem_post(&(shared.semaphores[requested_segment + EXTRA_SEMAPHORES]))) {
            report_and_exit("sem_post", SEM_POST_ERROR);
        }

        strcpy(requested_line, shared.requested_segment[no_requested_line]);
        sleep(0.02);
        fprintf(fp, "Request timestamp : ");
        print_timestamp(fp, timestamp_request, tm_request);

        gettimeofday(&timestampt_answer, NULL);
        tm_answer = localtime(&timestampt_answer.tv_sec);
        fprintf(fp, "Answer timestamp : ");
        print_timestamp(fp, timestampt_answer, tm_answer);
        
        fprintf(fp, "Child with pid %d is requested line <%u, %u>\n", getpid(), requested_segment, no_requested_line);
        fprintf(fp, "Answer : %s\n\n", requested_line);
        
        if (sem_wait(&(shared.semaphores[requested_segment + EXTRA_SEMAPHORES]))) {
            report_and_exit("sem_wait", SEM_WAIT_ERROR);
        }

        (*(shared.current_transaction)) ++;
        if (--(shared.readers[requested_segment]) == 0) {
            if (sem_post(&(shared.semaphores[SYNC_PARENT_ACCESS]))) {
                report_and_exit("sem_post", SEM_POST_ERROR);
            }
        }

        if (sem_post(&(shared.semaphores[requested_segment + EXTRA_SEMAPHORES]))) {
            report_and_exit("sem_post", SEM_POST_ERROR);
        }
    }
    
    if (fclose(fp) != 0) {
        report_and_exit("fclose", FCLOSE_ERROR);
    }

    return 0;
}
