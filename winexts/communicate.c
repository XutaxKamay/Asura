#include "main.h"

communicate_error_t
communicate_read__read_struct(task_t* task,
                              uintptr_t address,
                              communicate_read_t* communicate_read)
{
    if (c_copy_from_user(task,
                         communicate_read,
                         (ptr_t)address,
                         sizeof(communicate_read_t))
        <= 0)
    {
        c_printk_error("couldn't read communicate read struct from task "
                       "%i\n",
                       task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

communicate_error_t
communicate_read__write_struct(task_t* task,
                               uintptr_t address,
                               communicate_write_t* communicate_write)
{
    if (c_copy_from_user(task,
                         communicate_write,
                         (ptr_t)address,
                         sizeof(communicate_write_t))
        <= 0)
    {
        c_printk_error("couldn't read communicate write struct from task "
                       "%i\n",
                       task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

communicate_error_t communicate_process_cmd_read(task_t* task,
                                                 uintptr_t address)
{
    communicate_error_t error;
    communicate_read_t communicate_read;
    buffer_t temp_buffer;
    task_t* remote_task;

    init_buffer(&temp_buffer);

    error = communicate_read__read_struct(task,
                                          address,
                                          &communicate_read);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    if (communicate_read.vm_size > COMMUNICATE_MAX_BUFFER)
    {
        error = COMMUNICATE_ERROR_BUFFER_TOO_LARGE;
        goto out;
    }

    alloc_buffer(communicate_read.vm_size, &temp_buffer);

    remote_task = find_task_from_pid(communicate_read.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    if (c_copy_from_user(remote_task,
                         temp_buffer.addr,
                         (ptr_t)communicate_read.vm_remote_address,
                         temp_buffer.size)
        <= 0)
    {
        error = COMMUNICATE_ERROR_COPY_FROM;
        goto out;
    }

    if (c_copy_to_user(task,
                       (ptr_t)communicate_read.vm_local_address,
                       (ptr_t)temp_buffer.addr,
                       temp_buffer.size)
        <= 0)
    {
        error = COMMUNICATE_ERROR_COPY_TO;
        goto out;
    }

out:
    free_buffer(&temp_buffer);
    return error;
}

communicate_error_t communicate_process_cmd_write(task_t* task,
                                                  uintptr_t address)
{
    communicate_error_t error;
    struct communicate_write_struct communicate_write;
    buffer_t temp_buffer;
    task_t* remote_task;

    init_buffer(&temp_buffer);

    error = communicate_read__write_struct(task,
                                           address,
                                           &communicate_write);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    if (communicate_write.vm_size > COMMUNICATE_MAX_BUFFER)
    {
        error = COMMUNICATE_ERROR_BUFFER_TOO_LARGE;
        goto out;
    }

    alloc_buffer(communicate_write.vm_size, &temp_buffer);

    remote_task = find_task_from_pid(communicate_write.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    if (c_copy_from_user(task,
                         temp_buffer.addr,
                         (ptr_t)communicate_write.vm_local_address,
                         temp_buffer.size)
        <= 0)
    {
        error = COMMUNICATE_ERROR_COPY_FROM;
        goto out;
    }

    if (c_copy_to_user(remote_task,
                       (ptr_t)communicate_write.vm_remote_address,
                       (ptr_t)temp_buffer.addr,
                       temp_buffer.size)
        <= 0)
    {
        error = COMMUNICATE_ERROR_COPY_TO;
        goto out;
    }

out:
    free_buffer(&temp_buffer);
    return error;
}
