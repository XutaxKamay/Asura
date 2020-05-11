#ifndef COMMUNICATE_STRUCT
#define COMMUNICATE_STRUCT

#define COMMUNICATE_MAX_BUFFER (1 << 30)

typedef enum communicate_cmd
{
    COMMUNICATE_CMD_READ,
    COMMUNICATE_CMD_WRITE,
    COMMUNCIATE_CMD_CREATE_THREAD,
    COMMUNICATE_CMD_REMOTE_MMAP,
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
    COMMUNICATE_ERROR_MAX
} communicate_error_t;

typedef struct communicate_read_struct
{
    uintptr_t vm_local_address;
    uintptr_t vm_remote_address;
    uintptr_t vm_size;
    pid_t pid_target;
} communicate_read_t;

typedef struct communicate_write_struct
{
    uintptr_t vm_local_address;
    uintptr_t vm_remote_address;
    uintptr_t vm_size;
    pid_t pid_target;
} communicate_write_t;

typedef struct communicate_create_thread_struct
{
    uintptr_t routine_address;
    uintptr_t args;
    pid_t pid_target;
} communicate_create_thread_t;

typedef struct communicate_remote_nmap_struct
{
    uintptr_t address_result;
    uintptr_t wanted_address;
    uintptr_t vm_size;
    pid_t pid_target;
} communicate_remote_nmap_t;

typedef struct communicate_remote_munmap_struct
{
    uintptr_t remote_address;
    uintptr_t vm_size;
    pid_t pid_target;
} communicate_remote_munmap_t;

#endif // COMMUNICATE_STRUCT
