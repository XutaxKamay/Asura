#include "communicate.h"

DECLARE_WAIT_QUEUE_HEAD(g_wqh);
/* Thread for communicating */
struct task_struct *g_task_communicate = NULL;

static bool g_task_communicate_stop = false;
static copy_process_t copy_process = NULL;
static struct buffer_struct buffer_list_calls_copy_process;
// Just to initialize vma memory
static uint8_t g_zeros[PAGE_SIZE];

int find_copy_process(void)
{
	if (copy_process == NULL) {
		copy_process =
			(copy_process_t)kallsyms_lookup_name("copy_process");

		if (copy_process == NULL) {
			return 0;
		}
	}

	return 1;
}

int communicate_read_cmd(struct vm_area_struct *vma, int *read_size)
{
	int ret;
	struct communicate_read_struct read;
	struct buffer_struct buffer;

	init_buffer(&buffer);

	if (!copy_from_user(&read, (ptr_t)vma->vm_start, sizeof(read))) {
		ret = 0;
		goto out;
	}

	// That's too much don't you think?
	if (read.vm_size > PAGE_SIZE * 10) {
		ret = 0;
        goto out;
	}

	alloc_buffer(read.vm_size, &buffer);

	if (!copy_from_user(buffer.addr, (ptr_t)read.vm_remote_address,
			    read.vm_size)) {
		ret = 0;
		goto out;
	}

	if (!copy_to_user((ptr_t)read.vm_local_address, buffer.addr,
			  read.vm_size)) {
		ret = 0;
		goto out;
	}

	ret = 1;

out:
	free_buffer(&buffer);
	return ret;
}

int communicate_write_cmd(struct vm_area_struct *vma, int *read_size)
{
	int ret;
	struct communicate_write_struct write;
	struct buffer_struct buffer;

	init_buffer(&buffer);

	if (!copy_from_user(&write, (ptr_t)vma->vm_start, sizeof(write))) {
		ret = 0;
		goto out;
	}

	// That's too much don't you think?
	if (write.vm_size > PAGE_SIZE * 10) {
        ret = 0;
        goto out;
    }


	alloc_buffer(write.vm_size, &buffer);

	if (!copy_from_user(buffer.addr, (ptr_t)write.vm_local_address,
			    write.vm_size)) {
		ret = 0;
		goto out;
	}

	if (!copy_to_user((ptr_t)write.vm_remote_address, buffer.addr,
			  write.vm_size)) {
		ret = 0;
		goto out;
	}

	ret = 1;

out:
	free_buffer(&buffer);
	return ret;
}

int communicate_process_cmd(struct vm_area_struct *vma, pid_t pid, int cmd,
			    int *read_size)
{
	int ret;

	switch (cmd) {
	case COMMUNICATE_READ: {
		ret = communicate_read_cmd(vma, read_size);

		if (!ret) {
			c_printk("command read from task %i failed\n", pid);
			goto out;
		}

		*read_size += sizeof(struct communicate_read_struct);
		break;
	}
	case COMMUNICATE_WRITE: {
		ret = communicate_write_cmd(vma, read_size);

		if (!ret) {
			c_printk("command write from task %i failed\n", pid);
			goto out;
		}

		*read_size += sizeof(struct communicate_write_struct);
		break;
	}
	default: {
		c_printk("unknown command from task %i\n", pid);
		ret = 0;
	}
	}

out:
	return ret;
}

int communicate_read_special_vma_cmd(struct vm_area_struct *vma, pid_t pid)
{
	int number_of_cmds;
	int cmd_number;
	int cmd;
	int read_size;
	int ret;

	read_size = 0;

	// Maybe normal behavior
	if (vma == NULL) {
		ret = 1;
		goto out;
	}

	// First read the number of commands
	if (!copy_from_user(&number_of_cmds, (ptr_t)vma->vm_start,
			    sizeof(int))) {
		c_printk(
			"couldn't read the number of cmds copy_from_user on task %i\n",
			pid);
		ret = 0;
		goto out;
	}

	// Nothing to do.
	if (number_of_cmds <= COMMUNCIATE_ZERO_CMDS) {
		ret = 1;
		goto out;
	}

	read_size += sizeof(int);

	for (cmd_number = 0; cmd_number < number_of_cmds; cmd_number++) {
		// Then read the cmds
		if (!copy_from_user(&cmd, (ptr_t)(vma->vm_start + read_size),
				    sizeof(int) * cmd_number)) {
			c_printk(
				"couldn't read cmd %i with copy_from_user on task %i\n",
				cmd_number, pid);
			ret = 0;
			goto out;
		}

		read_size += sizeof(int);

		if (!communicate_process_cmd(vma, pid, cmd, &read_size)) {
			ret = 0;
			break;
		}
	}

	// Notify the task how much processed commands we have done.
	// Write it into a negative number so it's safe
	number_of_cmds = -((number_of_cmds - 1) - cmd_number);

	if (!copy_to_user(&number_of_cmds, (ptr_t)vma->vm_start, sizeof(int))) {
		c_printk("couldn't notify task %i with copy_from_user\n", pid);
		ret = 0;
		goto out;
	}

out:
	return ret;
}

void communicate_reset(struct vm_area_struct *vma, pid_t pid)
{
	if (vma == NULL) {
		return;
	}

	if (!copy_to_user((ptr_t)vma->vm_start, g_zeros, sizeof(g_zeros))) {
		c_printk(
			"couldn't reset vma from task %i with copy_from_user\n",
			pid);
	}
}

void communicate_with_task(struct task_struct *task)
{
	// Let's check the process who doesn't have our custom vma
	struct vm_area_struct *vma;

	if (task == NULL)
		return;

	vma = NULL;

	c_find_vma_from_task(task, &vma, MAGIC_ADDRESS);

	if (vma == NULL) {
		vma = remote_mmap(task->pid, MAGIC_ADDRESS, PROT_COMMUNICATE);

		if (vma != NULL) {
			c_printk(
				"allocated vma for communicating from task %i\n",
				task->pid);

			communicate_reset(vma, task->pid);
		}
	}

	if (!communicate_read_special_vma_cmd(vma, task->pid)) {
		communicate_reset(vma, task->pid);
	}
}

void communicate_with_tasks(void)
{
	struct task_struct *task;
	struct mm_struct *mm;

	for_each_process (task) {
		mm = get_task_mm(task);

		// Kernel thread?
		if (mm == NULL)
			mm = task->active_mm;

		if (mm == NULL)
			continue;

		communicate_with_task(task);
	}
}

void communicate_thread_with_tasks(void)
{
	// Wait for wake up
	DECLARE_WAITQUEUE(wq, current);
	add_wait_queue(&g_wqh, &wq);

	c_printk("[communicate] entering into loop\n");

	while (!g_task_communicate_stop) {
		communicate_with_tasks();

		// Sleep for non constant usage.
		msleep(100);
	}

	c_printk("closing thread running\n");

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&g_wqh, &wq);

	c_printk("closed thread to communicate with tasks\n");
}

/**
 * Hooking kernel
 */
static __latent_entropy struct task_struct *
new_copy_process(struct pid *pid, int trace, int node,
		 struct kernel_clone_args *args)
{
	struct task_struct *task;
	task = copy_process(pid, trace, node, args);

	communicate_with_task(current);
	communicate_with_task(task);

	return task;
}

void hook_callsof_copy_process(void)
{
	int ret;
	int count_calls;
	int i;
	char temp[64];
#ifdef BIT_64
	uint16_t movabs;
#else
	uint8_t movabs;
#endif
	uintptr_t *list_calls;

	ret = find_copy_process();

	if (!ret) {
		c_printk("couldn't find copy_process\n");
		return;
	}

	ret = convert_to_hexstring((uint8_t *)(&copy_process), sizeof(ptr_t),
				   temp, sizeof(temp));

	if (!ret) {
		c_printk(
			"couldn't convert address of copy_process to hexstring\n");
		return;
	}

	ret = scan_kernel("_text", "_fini", temp, strlen(temp) - 1,
			  &buffer_list_calls_copy_process);

	if (!ret) {
		c_printk(
			"didn't find any signatures of calls of copy_process\n");
		return;
	}

	count_calls = buffer_list_calls_copy_process.size / sizeof(ptr_t);

#ifdef BIT_64
	list_calls = (uintptr_t *)buffer_list_calls_copy_process.addr;

	for (i = 0; i < count_calls; i++) {
		movabs = *(uint16_t *)(list_calls[i] - 2);

		if (movabs == 0xb848) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = (ptr_t)new_copy_process;
		}
	}
#else
	for (i = 0; i < count_calls; i++) {
		movabs = *(uint16_t *)(list_calls[i] - 1);

		if (movabs == 0xb8) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = (ptr_t)new_copy_process;
		}
	}
#endif
}

void unhook_callsof_copy_process(void)
{
	int count_calls;
	int i;
#ifdef BIT_64
	uint16_t movabs;
#else
	uint8_t movabs;
#endif
	uintptr_t *list_calls;

	count_calls = buffer_list_calls_copy_process.size / sizeof(ptr_t);

#ifdef BIT_64
	list_calls = (uintptr_t *)buffer_list_calls_copy_process.addr;

	for (i = 0; i < count_calls; i++) {
		movabs = *(uint16_t *)(list_calls[i] - 2);

		if (movabs == 0xb848) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = copy_process;
		}
	}
#else
	for (i = 0; i < count_calls; i++) {
		movabs = *(uint16_t *)(list_calls[i] - 1);

		if (movabs == 0xb8) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = copy_process;
		}
	}
#endif
	free_buffer(&buffer_list_calls_copy_process);
}

void hook_kernel(void)
{
	c_printk("hooking kernel...\n");
	hook_callsof_copy_process();
	c_printk("hooked kernel\n");
}

void unhook_kernel(void)
{
	c_printk("unhooking kernel...\n");
	unhook_callsof_copy_process();
	c_printk("unhooked kernel\n");
}

void communicate_start_thread(void)
{
	memset(g_zeros, 0, sizeof(g_zeros));

	hook_kernel();

	if (g_task_communicate != NULL) {
		c_printk(
			"thread to communicate with others processes is already"
			"created?\n");
		return;
	}

	communicate_with_task(current);

	g_task_communicate = kthread_run((ptr_t)communicate_thread_with_tasks,
					 NULL, "communicate_thread");

	if (g_task_communicate) {
		// Increment counter.
		get_task_struct(g_task_communicate);

		msleep(1000);

		c_printk(
			"waking up created thread for communicating with tasks\n");

		wake_up(&g_wqh);

		c_printk(
			"successfully created thread for communicating with tasks\n");
	} else {
		c_printk(
			"failed to create thread for communicating with tasks\n");
	}
}

void communicate_kill_thread(void)
{
	// Stop communicate thread
	if (g_task_communicate != NULL) {
		g_task_communicate_stop = true;

		// Be sure to wake up the thread before stopping.
		wake_up(&g_wqh);

		// Wait for the thread termination
		kthread_stop(g_task_communicate);

		c_printk(
			"successfully stopped thread for communicating with tasks\n");

		// Release task
		__put_task_struct(g_task_communicate);
	}

	unhook_kernel();
}
