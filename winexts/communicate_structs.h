#ifndef COMMUNICATE_STRUCT
#define COMMUNICATE_STRUCT

#define COMMUNICATE_MAX_BUFFER (1 << 30)

typedef enum communicate_cmd
{
    COMMUNICATE_CMD_READ,
    COMMUNICATE_CMD_WRITE,
    COMMUNICATE_CMD_REMOTE_MMAP = 3, // The number two is damned
                                     // apparently.
    COMMUNICATE_CMD_REMOTE_MUNMAP
} communicate_cmd_t;

typedef enum communicate_error
{
    COMMUNICATE_ERROR_NONE,
    COMMUNICATE_ERROR_STRUCT_COPY_FROM,
    COMMUNICATE_ERROR_BUFFER_TOO_LARGE,
    COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND,
    COMMUNICATE_ERROR_COPY_FROM,
    COMMUNICATE_ERROR_COPY_TO,
    COMMUNICATE_ERROR_UNKNOWN_CMD,
    COMMUNICATE_ERROR_CMD_READ_HEADER_ERROR,
    COMMUNICATE_ERROR_MMAP_PGOFF_FAILED,
    COMMUNICATE_ERROR_VM_MUNMAP_FAILED,
    COMMUNICATE_ERROR_MAX
} communicate_error_t;

typedef struct communicate_read_struct
{
    uint64_t vm_local_address;
    uint64_t vm_remote_address;
    uint64_t vm_size;
    pid_t pid_target;
} communicate_read_t;

typedef struct communicate_write_struct
{
    uint64_t vm_local_address;
    uint64_t vm_remote_address;
    uint64_t vm_size;
    pid_t pid_target;
} communicate_write_t;

typedef struct communicate_remote_mmap_struct
{
    unsigned long vm_remote_address;
    unsigned long vm_real_remote_address;
    unsigned long vm_size;
    unsigned long prot;
    unsigned long flags;
    unsigned long fd;
    unsigned long offset;
    unsigned long pid_target;
} communicate_remote_mmap_t;

typedef struct communicate_remote_munmap_struct
{
    unsigned long vm_remote_address;
    unsigned long vm_size;
    pid_t pid_target;
} communicate_remote_munmap_t;

#endif // COMMUNICATE_STRUCT
