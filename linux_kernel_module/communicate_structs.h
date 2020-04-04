#ifndef COMMUNICATE_STRUCT
#define COMMUNICATE_STRUCT

#include "utils.h"

#define COMMUNICATE_MAX_BUFFER (1 << 30)

enum communicate_cmd
{
    COMMUNICATE_CMD_READ,
    COMMUNICATE_CMD_WRITE,
    COMMUNCIATE_CMD_CREATE_THREAD,
    COMMUNICATE_CMD_CLOSE_THREAD,
    COMMUNICATE_CMD_REMOTE_MMAP,
    COMMUNICATE_CMD_REMOTE_MUNMAP,
    COMMUNICATE_CMD_LIST_VMA
};

enum communicate_state
{
    COMMUNICATE_STATE_STOPPED,           // Communication is stopped.
    COMMUNICATE_STATE_READ_HEADER_ERROR, // Has header error.
    COMMUNICATE_STATE_PROCESSING, // The commands are being processed by
                                  // the module.
    COMMUNICATE_STATE_DONE,       // Commands are processed.
    COMMUNICATE_STATE_MAX
};

enum communicate_error
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
};

struct communicate_cmd_header_struct
{
    enum communicate_cmd type;
    ptr_t address;
};

struct communicate_header_struct
{
    enum communicate_state state;
    struct communicate_cmd_header_struct* cmds;
    int number_of_cmds;
    int command_number_error; // Used when a command isn't processed
    enum communicate_error error;
};

struct communicate_read_struct
{
    uintptr_t vm_local_address;
    uintptr_t vm_remote_address;
    uintptr_t vm_size;
    pid_t pid_target;
};

struct communicate_write_struct
{
    uintptr_t vm_local_address;
    uintptr_t vm_remote_address;
    uintptr_t vm_size;
    pid_t pid_target;
};

struct communicate_listvmas_struct
{
    uintptr_t local_address;
    uintptr_t size;
    pid_t pid_target;
};

struct communicate_create_thread_struct
{
    uintptr_t routine_address;
    uintptr_t args;
    pid_t pid_target;
};
struct communicate_close_thread_struct
{
    uintptr_t routine_address;
    pid_t pid_target;
};

struct communicate_remote_nmap_struct
{
    uintptr_t address_result;
    uintptr_t wanted_address;
    uintptr_t vm_size;
    pid_t pid_target;
};

struct communicate_remote_munmap_struct
{
    uintptr_t remote_address;
    pid_t pid_target;
};

#endif // COMMUNICATE_STRUCT
