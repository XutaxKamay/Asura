#include "communicate.h"

/* Thread for communicating */
struct mutex g_task_communicate_mutex;
struct task_struct* g_task_communicate = NULL;
bool g_task_communicate_stop           = false;

struct communicate_header_struct
communicate_read_header(struct task_struct* task, ptr_t address)
{
    struct communicate_header_struct communicate_header;

    if (c_copy_from_user(task,
                         address,
                         &communicate_header,
                         sizeof(communicate_header)))
    {
        c_printk("couldn't read communicate header from task %i\n",
                 task->pid);

        communicate_header.state = COMMUNICATE_STATE_READ_HEADER_ERROR;
    }

    return communicate_header;
}

enum communicate_error communicate_read__write_struct(
  struct task_struct* task,
  ptr_t address,
  struct communicate_write_struct* communicate_write)
{
    if (c_copy_from_user(task,
                         address,
                         communicate_write,
                         sizeof(struct communicate_write_struct)))
    {
        c_printk("couldn't read communicate write struct from task %i\n",
                 task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

enum communicate_error communicate_read__read_struct(
  struct task_struct* task,
  ptr_t address,
  struct communicate_read_struct* communicate_read)
{
    if (c_copy_from_user(task,
                         address,
                         communicate_read,
                         sizeof(struct communicate_read_struct)))
    {
        c_printk("couldn't read communicate read struct from task %i\n",
                 task->pid);

        return COMMUNICATE_ERROR_STRUCT_COPY_FROM;
    }

    return COMMUNICATE_ERROR_NONE;
}

enum communicate_error
communicate_process_cmd_read(struct task_struct* task, ptr_t address)
{
    enum communicate_error error;
    struct communicate_read_struct communicate_read;
    struct buffer_struct temp_buffer;
    struct task_struct* remote_task;

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
                         temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_FROM;
        goto out;
    }

    if (c_copy_to_user(task,
                       (ptr_t)communicate_read.vm_local_address,
                       (ptr_t)temp_buffer.addr,
                       temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_TO;
        goto out;
    }

out:
    free_buffer(&temp_buffer);
    return error;
}

enum communicate_error
communicate_process_cmd_write(struct task_struct* task, ptr_t address)
{
    enum communicate_error error;
    struct communicate_write_struct communicate_write;
    struct buffer_struct temp_buffer;
    struct task_struct* remote_task;

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
                         temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_FROM;
        goto out;
    }

    if (c_copy_from_user(remote_task,
                         (ptr_t)communicate_write.vm_remote_address,
                         (ptr_t)temp_buffer.addr,
                         temp_buffer.size))
    {
        error = COMMUNICATE_ERROR_COPY_TO;
        goto out;
    }

out:
    free_buffer(&temp_buffer);
    return error;
}

void communicate_process_cmds(
  struct task_struct* task,
  struct communicate_header_struct* communicate_header)
{
    struct communicate_cmd_header_struct* cmd_header;
    enum communicate_error error;
    int cmd_number;

    error = COMMUNICATE_ERROR_NONE;

    for (cmd_number = 0; cmd_number < communicate_header->number_of_cmds;
         cmd_number++)
    {
        cmd_header = &communicate_header->cmds[cmd_number];

        switch (cmd_header->type)
        {
            case COMMUNICATE_CMD_READ:
            {
                error = communicate_process_cmd_read(task,
                                                     cmd_header->address);
                break;
            }
            case COMMUNICATE_CMD_WRITE:
            {
                error = communicate_process_cmd_write(task,
                                                      cmd_header->address);
                break;
            }
            case COMMUNCIATE_CMD_CREATE_THREAD:
            {
                break;
            }
            case COMMUNICATE_CMD_LIST_VMA:
            {
                break;
            }
            default:
            {
                c_printk("communicate error, unknown command %i\n",
                         cmd_number);
                break;
            }
        }

        if (error != COMMUNICATE_ERROR_NONE)
        {
            goto out;
        }
    }

out:
    communicate_header->command_number_error = cmd_number;
    communicate_header->error                = error;
}

void communicate_reset(struct task_struct* task, ptr_t address)
{
    int state;

    state = COMMUNICATE_STATE_STOPPED;

    if (c_copy_to_user(task, address, &state, sizeof(state)))
    {
        c_printk("couldn't reset communication from task %i with "
                 "c_copy_to_user\n",
                 task->pid);
    }
}

void communicate_check_task(struct task_struct* task)
{
    struct vm_area_struct* vma;

    // No mm?
    if (task->mm == NULL && task->active_mm == NULL)
    {
        return;
    }

    vma = NULL;

    c_find_vma_from_task(task, &vma, MAGIC_ADDRESS);

    if (vma == NULL)
    {
        c_printk("did not found vma for communicating from task %i\n",
                 task->pid);
    }
}

void communicate_check_tasks(void)
{
    struct task_struct* task;

    for_each_process(task)
    {
        communicate_check_task(task);
    }
}

void communicate_with_task(struct task_struct* task)
{
    struct vm_area_struct* vma;
    struct communicate_header_struct communicate_header;

    if (task == NULL)
    {
        return;
    }

    // No mm?
    if (task->mm == NULL && task->active_mm == NULL)
    {
        return;
    }

    vma = NULL;

    c_find_vma_from_task(task, &vma, MAGIC_ADDRESS);

    if (vma == NULL)
    {
        vma = remote_mmap(task, MAGIC_ADDRESS, PROT_COMMUNICATE);

        if (vma != NULL)
        {
            //	c_printk(
            //	"allocated vma for communicating from task %i\n",
            //	task->pid);

            communicate_reset(task, (ptr_t)vma->vm_start);
        }
        else
        {
            c_printk("failed to allocate vma for communicating from task "
                     "%i\n",
                     task->pid);
        }
    }

    if (vma != NULL)
    {
        communicate_header
          = communicate_read_header(task, (ptr_t)vma->vm_start);

        if (communicate_header.state == COMMUNICATE_STATE_PROCESSING)
        {
            communicate_process_cmds(task, &communicate_header);
            communicate_header.state = COMMUNICATE_STATE_DONE;
        }
    }
}

void communicate_with_tasks(void)
{
    struct task_struct* task;

    for_each_process(task)
    {
        communicate_with_task(task);
    }
}

void communicate_thread_with_tasks(bool only_once)
{
    c_printk("[communicate] entering into loop\n");

    communicate_with_tasks();

    if (mutex_is_locked(&g_task_communicate_mutex))
    {
        mutex_unlock(&g_task_communicate_mutex);
    }

    while (!g_task_communicate_stop)
    {
        communicate_with_tasks();

        if (only_once)
        {
            g_task_communicate_stop = true;
        }
        else
        {
            msleep(200);
        }
    }

    c_printk("closing thread running\n");

    mutex_lock(&g_task_communicate_mutex);
}

void communicate_start_thread(bool only_once)
{
    if (g_task_communicate != NULL)
    {
        c_printk("thread to communicate with others processes is already"
                 "created?\n");
        return;
    }

    mutex_init(&g_task_communicate_mutex);
    g_task_communicate = kthread_run((ptr_t)communicate_thread_with_tasks,
                                     (ptr_t)only_once,
                                     "communicate_thread");

    if (g_task_communicate)
    {
        mutex_lock(&g_task_communicate_mutex);

        c_printk("successfully created thread for communicating with "
                 "tasks\n");
    }
    else
    {
        c_printk("failed to create thread for communicating with "
                 "tasks\n");
    }
}

void communicate_kill_thread(void)
{
    // Stop communicate thread
    if (g_task_communicate != NULL && !g_task_communicate_stop)
    {
        g_task_communicate_stop = true;

        // Wait a bit before..
        while (!mutex_is_locked(&g_task_communicate_mutex))
        {
            msleep(100);
        }

        mutex_unlock(&g_task_communicate_mutex);

        c_printk("successfully stopped thread for communicating with "
                 "tasks\n");

        g_task_communicate = NULL;
    }
}
