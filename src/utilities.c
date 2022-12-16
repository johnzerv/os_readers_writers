#include "common.h"

// Returns total lines of file
int get_input_lines(char *file) {
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(FOPEN_ERROR);
    }
        
    int lines = 0;
    char ch;

    ch = getc(fp);
    while (ch != EOF) {
        lines++;

        while ((ch = getc(fp)) != '\n' && ch != EOF);
    }

    if (fclose(fp) != 0) {
        perror("fclose");
        exit(FCLOSE_ERROR);
    }

    return lines;
}

void get_segment(FILE *fp, unsigned int no_requested_segment, char **segment, unsigned int no_segments,
                 unsigned int lines_per_segment, unsigned int total_lines) {
    char buffer[MAX_LINE_SIZE+1];

    for (int i = 0; i < no_requested_segment*lines_per_segment; i++) {
        assert((fgets(buffer, MAX_LINE_SIZE, fp)) != NULL);
    }

    for (int i = 0; i < lines_per_segment && i + no_requested_segment*lines_per_segment < total_lines; i++) {
        assert((fgets(segment[i], MAX_LINE_SIZE, fp)) != NULL);
    }

    rewind(fp);
}

void report_and_exit(char *error, ErrorCode code) {
    perror(error);
    exit(code);
}

void print_timestamp(FILE *fp, struct timeval tmval, struct tm *time_val) {
    fprintf(fp, "%02d-%02d-%02d " , time_val->tm_year + 1900, time_val->tm_mon + 1, time_val->tm_mday);
    fprintf(fp, "%02d:%02d:%02d:%04d\n", time_val->tm_hour, time_val->tm_min, time_val->tm_sec, (int) (tmval.tv_usec / 1000));
}

void free_segment(char **segment, int lines) {
    for (int i = 0; i < lines; i++) {
            free(segment[i]);
    }
    free(segment);
}

void free_shm(int shmid, void *shm) {
    // Mark the segment to be destroyed after detaching
    assert(shmctl(shmid, IPC_RMID, NULL) == 0);

    assert(shmdt(shm) == 0);
}

void free_semaphores(sem_t *semaphores, int limit) {
    for (int i = 0; i < limit; i++) {
        assert(sem_destroy(&(semaphores[i])) == 0);
    }
}

void free_resources(char *input_filename, char **segment, int lines, char **requested_seg, int shmid, void *shm, sem_t *sems, int limit) {
    free(input_filename);
    free_segment(segment, lines);
    free(requested_seg);
    free_shm(shmid, shm);
    free_semaphores(sems, limit);
}