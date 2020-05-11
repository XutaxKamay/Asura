#ifndef FILE_OPS
#define FILE_OPS

typedef struct file_operations file_operations_t;
typedef struct inode inode_t;
typedef struct file file_t;

extern file_operations_t g_fops;

int check_permissions(task_t* task);

int file_operation_open(inode_t* i, file_t* f);
int file_operation_release(inode_t* i, file_t* f);
long file_operation_ioctl(file_t* f, unsigned int n, unsigned long p);

#endif
