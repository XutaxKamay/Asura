#ifndef COMMUNICATE_STRUCT
#define COMMUNICATE_STRUCT

#define COMMUNICATE_MAX_BUFFER (1 << 30)
#define IO_MAGIC_NUMBER        'x'

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
    COMMUNICATE_ERROR_CLONE_FAILED,
    COMMUNICATE_ERROR_ACCESS_DENIED,
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
    uint64_t vm_remote_address;
    uint64_t vm_size;
    uint64_t prot;
    uint64_t flags;
    uint64_t fd;
    uint64_t offset;
    pid_t pid_target;
    uint64_t ret;
} communicate_remote_mmap_t;

typedef struct communicate_remote_munmap_struct
{
    uint64_t vm_remote_address;
    pid_t pid_target;
    int ret;
} communicate_remote_munmap_t;

typedef struct communicate_remote_clone_struct
{
    uint64_t flags;
    uint64_t pidfd;
    uint64_t child_tid;
    uint64_t parent_tid;
    int exit_signal;
    uint64_t stack;
    uint64_t stack_size;
    uint64_t tls;
    uint64_t set_tid;
    size_t set_tid_size;
    pid_t pid_target;
    int ret;
} communicate_remote_clone_t;

typedef enum communicate_cmd
{
    COMMUNICATE_CMD_READ = _IOWR(IO_MAGIC_NUMBER, 0, communicate_read_t*),
    COMMUNICATE_CMD_WRITE
    = _IOWR(IO_MAGIC_NUMBER, 1, communicate_write_t*),
    COMMUNICATE_CMD_REMOTE_MMAP
    = _IOWR(IO_MAGIC_NUMBER, 2, communicate_remote_mmap_t*),
    COMMUNICATE_CMD_REMOTE_MUNMAP
    = _IOWR(IO_MAGIC_NUMBER, 3, communicate_remote_munmap_t*),
    COMMUNICATE_CMD_REMOTE_CLONE
    = _IOWR(IO_MAGIC_NUMBER, 4, communicate_remote_clone_t*)
} communicate_cmd_t;

#endif // COMMUNICATE_STRUCT
