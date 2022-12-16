// #ifndef COMMON
// #define COMMON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

// Constants as indexes for semaphore's array
#define SYNC_REQUEST 0
#define SYNC_ANSWER 1
#define SYNC_PARENT_ACCESS 2
#define SYNC_CHILD_ACCESS 3

#define EXTRA_SEMAPHORES 4
#define MAX_LINE_SIZE 100
#define NO_ARGUMENTS 5


typedef enum ErrorCode {
  FOPEN_ERROR = -1,
  FCLOSE_ERROR = -2,
  SHMGET_ERROR = -3,
  SHMAT_ERROR = -4,
  SEM_INIT_ERROR = -5,
  FORK_ERROR = -6,
  SEM_POST_ERROR = -7,
  SEM_WAIT_ERROR = -8,
  MALLOC_ERROR = -9
} ErrorCode;


typedef struct {
    unsigned int *current_transaction;
    unsigned int *no_requested_segment;
    char **requested_segment;
    unsigned int *readers;
    sem_t *semaphores;
} SharedData;

int get_input_lines(char *);
void get_segment(FILE *fp, unsigned int no_requested_segment, char **segment,
                 unsigned int no_segments, unsigned int lines_per_segment, unsigned int total_lines);
// void free_recourses(SharedData *, int, char *, char *, int);
void report_and_exit(char *error, ErrorCode code);
void print_timestamp(FILE *fp, struct timeval tmval, struct tm *time_val);
void free_segment(char **segment, int lines);
void free_shm(int shmid, void *shm);
void free_semaphores(sem_t *semaphores, int limit);
void free_resources(char *input_filename, char **segment, int lines, char **requested_seg, int shmid, void *shm, sem_t *sems, int limit);

// #endif // COMMON