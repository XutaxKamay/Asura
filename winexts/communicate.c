#include "main.h"

communicate_error_t communicate_read__read_struct(
  task_t* task,
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

communicate_error_t communicate_read__write_struct(
  task_t* task,
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

communicate_error_t communicate_read__list_vmas_struct(
  task_t* task,
  uintptr_t address,
  communicate_list_vmas_t* communicate_list_vmas)
{
    if (c_copy_from_user(task,
                         communicate_list_vmas,
                         (ptr_t)address,
                         sizeof(communicate_list_vmas_t)))
    {
        c_printk_error("couldn't read communicate list vmas struct "
                       "from task "
                       "%i\n",
                       task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

communicate_error_t communicate_read__remote_mprotect_struct(
  task_t* task,
  uintptr_t address,
  communicate_remote_mprotect_t* communicate_remote_mprotect)
{
    if (c_copy_from_user(task,
                         communicate_remote_mprotect,
                         (ptr_t)address,
                         sizeof(communicate_remote_mprotect_t)))
    {
        c_printk_error("couldn't read communicate remote protect struct "
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

    /**
     * For now kernel threads are risky
     */
    if (remote_task->mm == NULL || remote_task->active_mm == NULL
        || (remote_task->mm != remote_task->active_mm))
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("can't read on a kernel thread due to security "
                       "risks (might also freeze your kernel) asked "
                       "task: %i\n",
                       remote_task->pid);
        goto out;
    }

    if (c_copy_from_user(remote_task,
                         temp_buffer.addr,
                         (ptr_t)communicate_read.vm_remote_address,
                         temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_FROM;
    }

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

    /**
     * For now kernel threads are risky
     */
    if (remote_task->mm == NULL || remote_task->active_mm == NULL
        || (remote_task->mm != remote_task->active_mm))
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("can't write on a kernel thread due to security "
                       "risks (might also freeze your kernel) asked "
                       "task: %i\n",
                       remote_task->pid);
        goto out;
    }

    if (c_copy_to_user(remote_task,
                       (ptr_t)communicate_write.vm_remote_address,
                       (ptr_t)temp_buffer.addr,
                       temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_TO;
    }

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

    /**
     * For now kernel threads are risky
     */
    if (remote_task->mm == NULL || remote_task->active_mm == NULL
        || (remote_task->mm != remote_task->active_mm))
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("can't munmap on a kernel thread due to security "
                       "risks (might also freeze your kernel) asked "
                       "task: %i\n",
                       remote_task->pid);
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

communicate_error_t communicate_process_cmd_remote_munmap(
  uintptr_t address)
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

    /**
     * For now kernel threads are risky
     */
    if (remote_task->mm == NULL || remote_task->active_mm == NULL
        || (remote_task->mm != remote_task->active_mm))
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("can't munmap on a kernel thread due to security "
                       "risks (might also freeze your kernel) asked "
                       "task: %i\n",
                       remote_task->pid);
        goto out;
    }

    communicate_remote_munmap.ret = c_munmap(
      remote_task,
      communicate_remote_munmap.vm_remote_address);

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
    communicate_error_t error;
    communicate_remote_clone_t communicate_remote_clone;
    task_t *remote_task, *old_current_task;
    struct kernel_clone_args clone_args;

    error = communicate_read__remote_clone_struct(
      current,
      address,
      &communicate_remote_clone);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    remote_task = find_task_from_pid(communicate_remote_clone.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    /**
     * For now kernel threads are risky
     */
    if (remote_task->mm == NULL || remote_task->active_mm == NULL
        || (remote_task->mm != remote_task->active_mm))
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("can't clone on a kernel thread due to security "
                       "risks (might also freeze your kernel) asked "
                       "task: %i\n",
                       remote_task->pid);
        goto out;
    }

    //     if (task_pt_regs(remote_task)->ax == -ENOSYS
    //         && task_pt_regs(remote_task)->orig_ax > 0)
    //     {
    //         error = COMMUNICATE_ERROR_ACCESS_DENIED;
    //         c_printk_error("task: %i is inside a syscall, can't
    //         fork\n",
    //                        remote_task->pid);
    //         goto out;
    //     }

    /**
     * We accept only these three states,
     * if not we create zombies processes that are unkillable and
     * might crash the kernel at the end.
     * The fork routine don't take this into account because it
     * assumes that the current task is always running, etc.
     * This might be not our case.
     * Also, check the exit_code, it might be a zombie process.
     */

    if (remote_task->state
          > (TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE | TASK_RUNNING)
        || remote_task->exit_code != 0)
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("the task %i needs to be running in order to"
                       " fork (state: %li, exit code: %i)\n",
                       remote_task->pid,
                       remote_task->state,
                       remote_task->exit_code);
        goto out;
    }

    memset(&clone_args, 0, sizeof(clone_args));

    clone_args.flags       = communicate_remote_clone.flags;
    clone_args.exit_signal = communicate_remote_clone.exit_signal;
    clone_args.stack       = communicate_remote_clone.stack;
    clone_args.stack_size  = communicate_remote_clone.stack_size;

    old_current_task = current;

    // current_task_switched = current;
    // task_attached_to      = remote_task;

    current_switch_to(remote_task, false);

    communicate_remote_clone.ret = c_do_fork(
      &clone_args,
      &communicate_remote_clone.regs,
      &communicate_remote_clone.regs_set);

    // current_task_switched = NULL;
    // task_attached_to      = NULL;

    current_switch_to(old_current_task, false);

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

communicate_error_t communicate_process_cmd_list_vmas(uintptr_t address)
{
    communicate_error_t error;
    communicate_list_vmas_t communicate_list_vmas;
    communicate_vma_t* communicate_vma;
    task_t* remote_task;
    buffer_t buffer;
    mm_t* mm = NULL;
    vm_area_t* vma;
    //     int vma_count;
    const char* vma_description = NULL;

    init_buffer(&buffer);

    error = communicate_read__list_vmas_struct(current,
                                               address,
                                               &communicate_list_vmas);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    remote_task = find_task_from_pid(communicate_list_vmas.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    /**
     * For now kernel threads are risky
     */
    if (remote_task->mm == NULL || remote_task->active_mm == NULL
        || (remote_task->mm != remote_task->active_mm))
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("can't write on a kernel thread due to security "
                       "risks (might also freeze your kernel) asked "
                       "task: %i\n",
                       remote_task->pid);
        goto out;
    }

    mm = remote_task->mm;

    /**
     * Just in case
     */
    down_read(&mm->mmap_sem);

    vma = mm->mmap;

    communicate_list_vmas.vma_count = 0;

    while (vma != NULL)
    {
        communicate_list_vmas.vma_count++;

        if (!realloc_buffer(sizeof(communicate_vma_t), &buffer))
        {
            up_read(&mm->mmap_sem);
            error = COMMUNICATE_ERROR_KERNEL_ALLOC_FAILED;
            goto out;
        }

        communicate_vma = &((communicate_vma_t*)buffer
                              .addr)[communicate_list_vmas.vma_count - 1];

        communicate_vma->vm_has_private_data = vma->vm_private_data ?
                                                 true :
                                                 false;
        communicate_vma->vm_start            = vma->vm_start;
        communicate_vma->vm_end              = vma->vm_end;
        communicate_vma->vm_flags            = vma->vm_flags;
        communicate_vma->vm_pgoff            = vma->vm_pgoff;
        communicate_vma->vm_page_prot        = vma->vm_page_prot.pgprot;
        communicate_vma->pkey                = vma_pkey(vma);

        if (vma->vm_file)
        {
            vma_description = d_path(&vma->vm_file->f_path,
                                     communicate_vma->vm_descriptor,
                                     COMMUNICATE_MAX_PATH);

            if (vma_description)
                goto found_description;
        }

        if (vma->vm_ops && vma->vm_ops->name)
        {
            vma_description = vma->vm_ops->name(vma);

            if (vma_description)
                goto found_description;
        }

        vma_description = arch_vma_name(vma);

        if (vma_description == NULL)
        {
            if (vma->vm_mm == NULL)
            {
                vma_description = "[vdso]";
                goto found_description;
            }

            if (vma->vm_start <= mm->brk && vma->vm_end >= mm->start_brk)
            {
                vma_description = "[heap]";
                goto found_description;
            }

            if (vma->vm_start <= vma->vm_mm->start_stack
                && vma->vm_end >= vma->vm_mm->start_stack)
            {
                vma_description = "[stack]";
                goto found_description;
            }
        }

        if (vma_description == NULL)
        {
            vma_description = "[anon]";
        }

    found_description:

        if (strlen(vma_description) <= COMMUNICATE_MAX_PATH)
        {
            strcpy(communicate_vma->vm_descriptor, vma_description);
        }
        else
        {
            strcpy(communicate_vma->vm_descriptor, "array is too low");
        }

        vma = vma->vm_next;
    }

    up_read(&mm->mmap_sem);

    if (communicate_list_vmas.vma_count
        > communicate_list_vmas.vma_max_count)
    {
        error = COMMUNICATE_ERROR_LIST_VMAS_NOT_ENOUGH_MEMORY;
        c_printk_error("can't list all vmas on task %i, there is %i vmas "
                       "when max vmas count is %i\n",
                       remote_task->pid,
                       communicate_list_vmas.vma_count,
                       communicate_list_vmas.vma_max_count);
        goto out;
    }

    if (c_copy_to_user(current,
                       communicate_list_vmas.vmas,
                       buffer.addr,
                       buffer.size))
    {
        c_printk_error("couldn't write communicate vmas "
                       "from task "
                       "%i\n",
                       current->pid);

        error = COMMUNICATE_ERROR_COPY_TO;
        goto out;
    }

    //     for (vma_count = 0; vma_count <
    //     communicate_list_vmas.vma_count;
    //          vma_count++)
    //     {
    //         communicate_vma =
    //         &((communicate_vma_t*)buffer.addr)[vma_count];
    //
    //         c_printk_info("vma: %llX-%llX %s\n",
    //                       communicate_vma->vm_start,
    //                       communicate_vma->vm_end,
    //                       communicate_vma->vm_descriptor);
    //     }
    //
    //     memset(buffer.addr, 0, buffer.size);
    //
    //     if (c_copy_from_user(current,
    //                          buffer.addr,
    //                          communicate_list_vmas.vmas,
    //                          buffer.size))
    //     {
    //         c_printk_error("couldn't read communicate vmas "
    //                        "from task "
    //                        "%i\n",
    //                        current->pid);
    //
    //         error = COMMUNICATE_ERROR_COPY_FROM;
    //         goto out;
    //     }
    //
    //     for (vma_count = 0; vma_count <
    //     communicate_list_vmas.vma_count;
    //          vma_count++)
    //     {
    //         communicate_vma =
    //         &((communicate_vma_t*)buffer.addr)[vma_count];
    //
    //         c_printk_info("vma: %llX-%llX %s\n",
    //                       communicate_vma->vm_start,
    //                       communicate_vma->vm_end,
    //                       communicate_vma->vm_descriptor);
    //     }

    if (c_copy_to_user(current,
                       (ptr_t)address,
                       (ptr_t)&communicate_list_vmas,
                       sizeof(communicate_list_vmas_t)))
    {
        c_printk_error("couldn't write communicate list vmas "
                       "struct "
                       "from task "
                       "%i\n",
                       current->pid);

        error = COMMUNICATE_ERROR_COPY_TO;
        goto out;
    }

out:
    free_buffer(&buffer);
    return error;
}

communicate_error_t communicate_process_cmd_remote_mprotect(
  uintptr_t address)
{
    communicate_error_t error;
    communicate_remote_mprotect_t communicate_remote_mprotect;
    task_t* remote_task;

    error = communicate_read__remote_mprotect_struct(
      current,
      address,
      &communicate_remote_mprotect);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        goto out;
    }

    remote_task = find_task_from_pid(
      communicate_remote_mprotect.pid_target);

    if (remote_task == NULL)
    {
        error = COMMUNICATE_ERROR_TARGET_PID_NOT_FOUND;
        goto out;
    }

    /**
     * For now kernel threads are risky
     */
    if (remote_task->mm == NULL || remote_task->active_mm == NULL
        || (remote_task->mm != remote_task->active_mm))
    {
        error = COMMUNICATE_ERROR_ACCESS_DENIED;
        c_printk_error("can't write on a kernel thread due to security "
                       "risks (might also freeze your kernel) asked "
                       "task: %i\n",
                       remote_task->pid);
        goto out;
    }

    communicate_remote_mprotect.ret = c_mprotect(
      remote_task,
      communicate_remote_mprotect.address,
      communicate_remote_mprotect.size,
      communicate_remote_mprotect.wanted_flags,
      communicate_remote_mprotect.pkey);

    if (c_copy_to_user(current,
                       (ptr_t)address,
                       (ptr_t)&communicate_remote_mprotect,
                       sizeof(communicate_remote_mprotect_t)))
    {
        c_printk_error("couldn't write communicate remote protect "
                       "struct "
                       "from task "
                       "%i\n",
                       current->pid);

        error = COMMUNICATE_ERROR_COPY_TO;
    }

out:
    return error;
}

