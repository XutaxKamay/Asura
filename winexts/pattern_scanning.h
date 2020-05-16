#ifndef PATTERN_SCANNING_H
#define PATTERN_SCANNING_H

typedef struct pattern_result_struct
{
    uintptr_t* addrs;
    int count;
} pattern_result_t;

void init_pattern_result(pattern_result_t* pattern_result, size_t count);
bool realloc_pattern_result(pattern_result_t* pattern_result,
                            size_t add_count);
void free_pattern_result(pattern_result_t* pattern_result);
int scan_task(task_t* task,
              char* pattern,
              int len,
              pattern_result_t* pattern_result);
int scan_kernel(char* start,
                char* end,
                char* pattern,
                int len,
                pattern_result_t* pattern_result);
int scan_pattern(uintptr_t start,
                 uintptr_t end,
                 char* pattern,
                 int len,
                 pattern_result_t* pattern_result);

#endif
