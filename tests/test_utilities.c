#include "common.h"
#include "acutest.h"

void test_utilities(){ 
    char filename[] = "./texts/text.txt";
    unsigned int lines_per_segment = 3, no_requested_segment = 2;
    unsigned int total_lines = get_input_lines(filename), lines_div = total_lines / lines_per_segment;

    unsigned int total_segments = (total_lines % lines_per_segment == 0) ? lines_div : lines_div + 1;

    FILE *fp = fopen(filename, "r");
    assert(fp != NULL);

    char **segment = malloc(sizeof(char*) * lines_per_segment);
    assert(segment != NULL);

    for (int i = 0; i < lines_per_segment; i++) {
        segment[i] = malloc(sizeof(char) * MAX_LINE_SIZE+1);
        assert(segment[i] != NULL);
    }

    get_segment(fp, no_requested_segment, segment, total_segments, lines_per_segment, total_lines);
    TEST_ASSERT(!strcmp(segment[0], "86930\r\n"));
    TEST_ASSERT(!strcmp(segment[1], "758209\r\n"));
    TEST_ASSERT(!strcmp(segment[2], "proeiwk\r\n"));

    // Check edge case : <total lines> mod <lines per segment> != 0
    no_requested_segment = 4;
    get_segment(fp, no_requested_segment, segment, total_segments, lines_per_segment, total_lines);
    TEST_ASSERT(!strcmp(segment[0], "hello\r\n"));
    TEST_ASSERT(!strcmp(segment[1], "lastline"));

    for (int i = 0; i < lines_per_segment; i++) {
        free(segment[i]);
    }
    free(segment);

    fclose(fp);
}

TEST_LIST = {{"test_utilities", test_utilities}, {NULL, NULL}};