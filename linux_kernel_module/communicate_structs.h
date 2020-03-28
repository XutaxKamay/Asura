#ifndef COMMUNICATE_STRUCT
#define COMMUNICATE_STRUCT

#include "utils.h"

#define COMMUNICATE_MAX_BUFFER_READ (1 << 30)

enum communicate_cmd {
	COMMUNICATE_CMD_READ,
	COMMUNICATE_CMD_WRITE,
	COMMUNCIATE_CMD_CREATE_THREAD,
	COMMUNICATE_CMD_LIST_VMA
};

enum communicate_state {
	COMMUNICATE_STATE_STOPPED, // Communication is stopped.
	COMMUNICATE_STATE_READ_HEADER_ERROR, // Has header error.
	COMMUNICATE_STATE_PROCESSING, // The commands are being processed by the module.
	COMMUNICATE_STATE_DONE, // Commands are processed.
	COMMUNICATE_STATE_MAX
};

enum communicate_error {
	COMMUNICATE_ERROR_NONE,
    COMMUNICATE_ERROR_STRUCT_COPY_FROM,
    COMMUNICATE_ERROR_BUFFER_TOO_LARGE,
	COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND,
    COMMUNICATE_ERROR_COPY_FROM,
    COMMUNICATE_ERROR_COPY_TO,
	COMMUNICATE_ERROR_MAX
};

struct communicate_cmd_header_struct {
	enum communicate_cmd type;
	ptr_t address;
};

struct communicate_header_struct {
	int number_of_cmds;
	struct communicate_cmd_header_struct *cmds;
	enum communicate_state state;
	int command_number_error; // Used when a command isn't processed
	enum communicate_error error;
};

struct communicate_write_struct {
	uintptr_t vm_local_address;
	uintptr_t vm_remote_address;
	uintptr_t vm_size;
	pid_t pid_target;
};

struct communicate_read_struct {
	uintptr_t vm_local_address;
	uintptr_t vm_remote_address;
	uintptr_t vm_size;
	pid_t pid_target;
};

#endif // COMMUNICATE_STRUCT
