#ifndef COMMUNICATE_STRUCT
    #define COMMUNICATE_STRUCT

    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <memory.h>
    #include <string.h>
    #include <unistd.h>

    #define COMMUNICATE_MAX_BUFFER (1 << 30)

typedef int pid_t;
typedef void* ptr_t;

enum communicate_cmd
{
    COMMUNICATE_CMD_READ,
    COMMUNICATE_CMD_WRITE,
    COMMUNCIATE_CMD_CREATE_THREAD,
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

#endif // COMMUNICATE_STRUCT

struct communicate_header_struct* communicate_header
  = (struct communicate_header_struct*)0x13370000;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Not enough arguments\n");
        return 0;
    }

    int temp = 1337;
    struct communicate_cmd_header_struct* cmd_header
      = (struct communicate_cmd_header_struct*)malloc(
        sizeof(struct communicate_cmd_header_struct));

    struct communicate_write_struct* communicate_write
      = (struct communicate_write_struct*)malloc(
        sizeof(struct communicate_write_struct));

    printf("pid_target %s\n", argv[1]);
    communicate_write->pid_target       = atoi(argv[1]);
    communicate_write->vm_local_address = (uintptr_t)&temp;

    printf("vm_remote_address %s\n", argv[2]);
    communicate_write->vm_remote_address = strtoll(argv[2],
                                                   (char**)NULL,
                                                   16);
    communicate_write->vm_size           = sizeof(temp);

    communicate_header->state = COMMUNICATE_STATE_PROCESSING;
    communicate_header->cmds  = malloc(
      sizeof(struct communicate_cmd_header_struct) * 1);
    communicate_header->cmds[0].type    = COMMUNICATE_CMD_WRITE;
    communicate_header->cmds[0].address = communicate_write;
    communicate_header->number_of_cmds  = 1;

    while (communicate_header->state != COMMUNICATE_STATE_DONE)
    {
        sleep(1);
    }

    printf("state %i\n", communicate_header->state);

    free(cmd_header);
    free(communicate_write);
    free(communicate_header->cmds);

    return 0;
}
