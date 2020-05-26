#ifndef COMMUNICATE_STRUCT_H
#define COMMUNICATE_STRUCT_H

#define COMMUNICATE_MAX_BUFFER (1 << 30)
#define IO_MAGIC_NUMBER        'x'

#define COMMUNICATE_MAX_PATH 4096

/**
 * These flags are useful if someone wants to unprotect our memory
 */
#define C_PROT_MAY_EXEC  (PROT_EXEC << 4)
#define C_PROT_MAY_READ  (PROT_READ << 4)
#define C_PROT_MAY_WRITE (PROT_WRITE << 4)
#define C_PROT_MAY_SHARE ((1 << 3) << 4)

/**
 * ptrace pt_regs for settings registers
 * This is the same structure as pt_regs so it's safe.
 */
typedef struct communicate_regs
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t bp;
    uint64_t bx;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t ax;
    uint64_t cx;
    uint64_t dx;
    uint64_t si;
    uint64_t di;
    uint64_t orig_ax;
    uint64_t ip;
    uint64_t cs;
    uint64_t flags;
    uint64_t sp;
    uint64_t ss;
} communicate_regs_t;

/**
 * Determinates if we should set the registers or not
 * For example we might not wanting to set the stack
 * as copy_process is doing it already for you,
 * but you can do it.
 */
typedef struct communicate_regs_set
{
    bool r15;
    bool r14;
    bool r13;
    bool r12;
    bool bp;
    bool bx;
    bool r11;
    bool r10;
    bool r9;
    bool r8;
    bool ax;
    bool cx;
    bool dx;
    bool si;
    bool di;
    bool orig_ax;
    bool ip;
    bool cs;
    bool flags;
    bool sp;
    bool ss;
} communicate_regs_set_t;

typedef enum communicate_error
{
    COMMUNICATE_ERROR_NONE,
    COMMUNICATE_ERROR_STRUCT_COPY_FROM,
    COMMUNICATE_ERROR_BUFFER_TOO_LARGE,
    COMMUNICATE_ERROR_KERNEL_ALLOC_FAILED,
    COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND,
    COMMUNICATE_ERROR_COPY_FROM,
    COMMUNICATE_ERROR_COPY_TO,
    COMMUNICATE_ERROR_UNKNOWN_CMD,
    COMMUNICATE_ERROR_CMD_READ_HEADER_ERROR,
    COMMUNICATE_ERROR_MMAP_PGOFF_FAILED,
    COMMUNICATE_ERROR_VM_MUNMAP_FAILED,
    COMMUNICATE_ERROR_VM_MMAP_FAILED,
    COMMUNICATE_ERROR_CLONE_FAILED,
    COMMUNICATE_ERROR_ACCESS_DENIED,
    COMMUNICATE_ERROR_LIST_VMAS_NOT_ENOUGH_MEMORY,
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
    uint64_t stack;
    uint64_t stack_size;
    int exit_signal;
    pid_t pid_target;
    pid_t ret;
    communicate_regs_t regs;
    communicate_regs_set_t regs_set;
} communicate_remote_clone_t;

typedef struct communicate_vma_struct
{
    uint64_t vm_start;
    uint64_t vm_end;
    uint64_t vm_flags;
    uint64_t vm_page_prot;
    uint64_t vm_pgoff;
    int pkey;
    char vm_descriptor[COMMUNICATE_MAX_PATH];
    bool vm_has_private_data;
} communicate_vma_t;

typedef struct communicate_list_vmas_struct
{
    communicate_vma_t* vmas;
    int vma_count;     /* Counts vma listed above */
    int vma_max_count; /* Max allocated by the user */
    pid_t pid_target;
} communicate_list_vmas_t;

typedef struct communicate_remote_mprotect_struct
{
    uint64_t address;
    uint64_t size;
    int wanted_flags;
    int pkey;
    pid_t pid_target;
    int ret;
} communicate_remote_mprotect_t;

typedef enum communicate_cmd
{
    COMMUNICATE_CMD_READ = _IOWR(IO_MAGIC_NUMBER, 0, communicate_read_t*),
    COMMUNICATE_CMD_WRITE          = _IOWR(IO_MAGIC_NUMBER,
                                  1,
                                  communicate_write_t*),
    COMMUNICATE_CMD_REMOTE_MMAP    = _IOWR(IO_MAGIC_NUMBER,
                                        2,
                                        communicate_remote_mmap_t*),
    COMMUNICATE_CMD_REMOTE_MUNMAP  = _IOWR(IO_MAGIC_NUMBER,
                                          3,
                                          communicate_remote_munmap_t*),
    COMMUNICATE_CMD_REMOTE_CLONE   = _IOWR(IO_MAGIC_NUMBER,
                                         4,
                                         communicate_remote_clone_t*),
    COMMUNICATE_CMD_LIST_VMAS      = _IOWR(IO_MAGIC_NUMBER,
                                      5,
                                      communicate_list_vmas_t*),
    COMMUNICATE_CMD_REMOTE_PROTECT = _IOWR(IO_MAGIC_NUMBER,
                                           6,
                                           communicate_remote_mprotect_t*)
} communicate_cmd_t;

#endif
