#include "main.h"

communicate_error_t
communicate_read__read_struct(task_t* task,
                              uintptr_t address,
                              communicate_read_t* communicate_read)
{
    if (c_copy_from_user(task,
                         communicate_read,
                         (ptr_t)address,
                         sizeof(communicate_read_t)))
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
                         sizeof(communicate_write_t)))
    {
        c_printk_error("couldn't read communicate write struct from task "
                       "%i\n",
                       task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

communicate_error_t communicate_read__remote_mmap_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_mmap_t* communicate_remote_mmap)
{
    if (c_copy_from_user(task,
                         communicate_remote_mmap,
                         (ptr_t)address,
                         sizeof(communicate_remote_mmap_t)))
    {
        c_printk_error("couldn't read communicate remote mmap struct "
                       "from task "
                       "%i\n",
                       task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

communicate_error_t communicate_read__remote_munmap_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_munmap_t* communicate_remote_munmap)
{
    if (c_copy_from_user(task,
                         communicate_remote_munmap,
                         (ptr_t)address,
                         sizeof(communicate_remote_munmap_t)))
    {
        c_printk_error("couldn't read communicate remote munmap struct "
                       "from task "
                       "%i\n",
                       task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

communicate_error_t communicate_read__remote_clone_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_clone_t* communicate_remote_clone)
{
    if (c_copy_from_user(task,
                         communicate_remote_clone,
                         (ptr_t)address,
                         sizeof(communicate_remote_clone_t)))
    {
        c_printk_error("couldn't read communicate remote clone struct "
                       "from task "
                       "%i\n",
                       task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

communicate_error_t communicate_process_cmd_read(uintptr_t address)
{
    communicate_error_t error;
    communicate_read_t communicate_read;
    buffer_t temp_buffer;
    task_t* remote_task;

    init_buffer(&temp_buffer);

    error = communicate_read__read_struct(current,
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
                         temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_FROM;
    }

    c_put_task_struct(remote_task);

    if (c_copy_to_user(current,
                       (ptr_t)communicate_read.vm_local_address,
                       (ptr_t)temp_buffer.addr,
                       temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_TO;
    }

out:
    free_buffer(&temp_buffer);
    return error;
}

communicate_error_t communicate_process_cmd_write(uintptr_t address)
{
    communicate_error_t error;
    communicate_write_t communicate_write;
    buffer_t temp_buffer;
    task_t* remote_task;

    init_buffer(&temp_buffer);

    error = communicate_read__write_struct(current,
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

    if (c_copy_from_user(current,
                         temp_buffer.addr,
                         (ptr_t)communicate_write.vm_local_address,
                         temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_FROM;
    }

    remote_task = find_task_from_pid(communicate_write.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    if (c_copy_to_user(remote_task,
                       (ptr_t)communicate_write.vm_remote_address,
                       (ptr_t)temp_buffer.addr,
                       temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_TO;
    }

    c_put_task_struct(remote_task);

out:
    free_buffer(&temp_buffer);
    return error;
}

communicate_error_t communicate_process_cmd_remote_mmap(uintptr_t address)
{
    communicate_error_t error;
    communicate_remote_mmap_t communicate_remote_mmap;
    task_t* remote_task;
    vm_area_t* vma;

    error = communicate_read__remote_mmap_struct(current,
                                                 address,
                                                 &communicate_remote_mmap);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    remote_task = find_task_from_pid(communicate_remote_mmap.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    vma = c_mmap(remote_task,
                 communicate_remote_mmap.vm_remote_address,
                 communicate_remote_mmap.vm_size,
                 communicate_remote_mmap.prot);

    if (vma != NULL)
    {
        communicate_remote_mmap.ret = vma->vm_start;
    }
    else
    {
        communicate_remote_mmap.ret = 0;
    }

    c_put_task_struct(remote_task);

    if (c_copy_to_user(current,
                       (ptr_t)address,
                       (ptr_t)&communicate_remote_mmap,
                       sizeof(communicate_remote_mmap_t)))
    {
        c_printk_error("couldn't write communicate remote mmap "
                       "struct "
                       "from task "
                       "%i\n",
                       current->pid);

        error = COMMUNICATE_ERROR_COPY_TO;
    }

out:
    return error;
}

communicate_error_t
communicate_process_cmd_remote_munmap(uintptr_t address)
{
    communicate_error_t error;
    communicate_remote_munmap_t communicate_remote_munmap;
    task_t* remote_task;

    error = communicate_read__remote_munmap_struct(
      current,
      address,
      &communicate_remote_munmap);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    remote_task = find_task_from_pid(communicate_remote_munmap.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    communicate_remote_munmap.ret
      = c_munmap(remote_task,
                 communicate_remote_munmap.vm_remote_address);


    c_put_task_struct(remote_task);

    if (communicate_remote_munmap.ret < 0)
    {
        error = COMMUNICATE_ERROR_VM_MUNMAP_FAILED;
    }

    if (c_copy_to_user(current,
                       (ptr_t)address,
                       (ptr_t)&communicate_remote_munmap,
                       sizeof(communicate_remote_munmap_t)))
    {
        c_printk_error("couldn't write communicate remote munmap "
                       "struct "
                       "from task "
                       "%i\n",
                       current->pid);

        error = COMMUNICATE_ERROR_COPY_TO;
    }

out:
    return error;
}

communicate_error_t communicate_process_cmd_remote_clone(uintptr_t address)
{
    mm_segment_t old_fs;
    communicate_error_t error;
    communicate_remote_clone_t communicate_remote_clone;
    task_t* remote_task;
    struct kernel_clone_args clone_args;

    error
      = communicate_read__remote_clone_struct(current,
                                              address,
                                              &communicate_remote_clone);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    memcpy(&clone_args,
           &communicate_remote_clone,
           sizeof(struct kernel_clone_args));

    remote_task = find_task_from_pid(communicate_remote_clone.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    communicate_remote_clone.ret = -1;

    c_put_task_struct(remote_task);

    if (c_copy_to_user(current,
                       (ptr_t)address,
                       (ptr_t)&communicate_remote_clone,
                       sizeof(communicate_remote_clone_t)))
    {
        c_printk_error("couldn't write communicate remote clone "
                       "struct "
                       "from task "
                       "%i\n",
                       current->pid);

        error = COMMUNICATE_ERROR_COPY_TO;
    }

out:
    return error;
}
