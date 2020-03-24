#include "communicate.h"

DECLARE_WAIT_QUEUE_HEAD(g_wqh);
/* Thread for communicating */
struct task_struct *g_task_communicate = NULL;

static bool g_task_communicate_stop = false;
static copy_process_t copy_process = NULL;
static exec_binprm_t exec_binprm = NULL;
static struct buffer_struct buffer_list_calls_copy_process;
static struct buffer_struct buffer_list_calls_exec_binprm;

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

int find_exec_binprm(void)
{
	if (exec_binprm == NULL) {
		exec_binprm =
			(exec_binprm_t)kallsyms_lookup_name("exec_binprm");

		if (exec_binprm == NULL) {
			return 0;
		}
	}

	return 1;
}

void communicate_reset(struct task_struct *task, struct vm_area_struct *vma)
{
	struct buffer_struct buffer;

	init_buffer(&buffer);

	if (vma == NULL) {
		return;
	}

	alloc_buffer((size_t)(vma->vm_end - vma->vm_start), &buffer);
	memset(buffer.addr, 0x00, buffer.size);

	if (c_copy_to_user(task, (ptr_t)vma->vm_start, buffer.addr,
			   buffer.size)) {
		c_printk(
            "couldn't reset vma from task %i with c_copy_to_user\n",
			task->pid);
		goto out;
	}

	c_printk("reset buffer %li from task %i\n", vma->vm_end - vma->vm_start,
		 task->pid);

out:
	free_buffer(&buffer);
}

void communicate_with_task(struct task_struct *task)
{
	struct vm_area_struct *vma;

	// Let's check the process who doesn't have our custom vma
	if (task == NULL || task == g_task_communicate)
		return;

	// No mm?
	if (task->mm == NULL && task->active_mm == NULL)
		return;

	vma = NULL;

	c_find_vma_from_task(task, &vma, MAGIC_ADDRESS);

	if (vma == NULL) {
		vma = remote_mmap(task, MAGIC_ADDRESS, PROT_COMMUNICATE);

		if (vma != NULL) {
			c_printk(
				"allocated vma for communicating from task %i\n",
				task->pid);

			communicate_reset(task, vma);
		}
	}
}

void communicate_with_tasks(void)
{
	struct task_struct *task;

	for_each_process (task) {
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
		msleep(200);
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
	struct task_struct *task, *next_task;
	task = copy_process(pid, trace, node, args);

	c_printk("copy_process: created task with pid %i\n", task->pid);

	// Just in case, doesn't hurt to check
	communicate_with_task(current);
	communicate_with_task(task);
	communicate_with_task(current->parent);
	communicate_with_task(task->parent);

	for_each_process (next_task) {
		if (next_task == task || next_task == task->parent ||
		    next_task == current || next_task == current->parent) {
			continue;
		}
		communicate_with_task(next_task);
	}

	return task;
}

static int new_exec_binprm(struct linux_binprm *bprm)
{
	int result;
	struct task_struct *next_task;

	result = exec_binprm(bprm);

	c_printk("exec_binprm: created task with pid %i\n", current->pid);

	communicate_with_task(current);
	// Just in case.
	communicate_with_task(current->parent);

	for_each_process (next_task) {
		if (next_task == current || next_task == current->parent) {
			continue;
		}
		communicate_with_task(next_task);
	}

	return result;
}

void hook_callsof_copy_process(void)
{
	int ret;
	int count_calls;
	int i;
	char temp[64];
#ifdef BIT_64
	uint16_t movabs_ax;
#else
	uint8_t movabs_ax;
#endif
	uintptr_t *list_calls;

	init_buffer(&buffer_list_calls_copy_process);

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

	ret = scan_kernel("_text", "_end", temp, strlen(temp) - 1,
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
		movabs_ax = *(uint16_t *)(list_calls[i] - 2);

		if (movabs_ax == 0xb848) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = (ptr_t)new_copy_process;
		}
	}
#else
	for (i = 0; i < count_calls; i++) {
		movabs_ax = *(uint16_t *)(list_calls[i] - 1);

		if (movabs_ax == 0xb8) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = (ptr_t)new_copy_process;
		}
	}
#endif
}

void hook_callsof_exec_binprm(void)
{
	int ret;
	int count_calls;
	int i;
	char temp[64];
#ifdef BIT_64
	uint16_t movabs_ax;
#else
	uint8_t movabs_ax;
#endif
	uintptr_t *list_calls;

	init_buffer(&buffer_list_calls_exec_binprm);

	ret = find_exec_binprm();

	if (!ret) {
		c_printk("couldn't find exec_binprm\n");
		return;
	}

	ret = convert_to_hexstring((uint8_t *)(&exec_binprm), sizeof(ptr_t),
				   temp, sizeof(temp));

	if (!ret) {
		c_printk(
			"couldn't convert address of exec_binprm to hexstring\n");
		return;
	}

	ret = scan_kernel("_text", "_end", temp, strlen(temp) - 1,
			  &buffer_list_calls_exec_binprm);

	if (!ret) {
		c_printk(
			"didn't find any signatures of calls of exec_binprm\n");
		return;
	}

	count_calls = buffer_list_calls_exec_binprm.size / sizeof(ptr_t);

#ifdef BIT_64
	list_calls = (uintptr_t *)buffer_list_calls_exec_binprm.addr;

	for (i = 0; i < count_calls; i++) {
		movabs_ax = *(uint16_t *)(list_calls[i] - 2);

		if (movabs_ax == 0xb848) {
			c_printk("removing call for exec_binprm at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = (ptr_t)new_exec_binprm;
		}
	}
#else
	for (i = 0; i < count_calls; i++) {
		movabs_ax = *(uint16_t *)(list_calls[i] - 1);

		if (movabs_ax == 0xb8) {
			c_printk("removing call for exec_binprm at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = (ptr_t)new_exec_binprm;
		}
	}
#endif
}

void unhook_callsof_copy_process(void)
{
	int count_calls;
	int i;
#ifdef BIT_64
	uint16_t movabs_ax;
#else
	uint8_t movabs_ax;
#endif
	uintptr_t *list_calls;

	if (buffer_list_calls_copy_process.addr == NULL) {
		c_printk("nothing to unhook for copy_process\n");
		goto out;
	}

	count_calls = buffer_list_calls_copy_process.size / sizeof(ptr_t);

#ifdef BIT_64
	list_calls = (uintptr_t *)buffer_list_calls_copy_process.addr;

	for (i = 0; i < count_calls; i++) {
		movabs_ax = *(uint16_t *)(list_calls[i] - 2);

		if (movabs_ax == 0xb848) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = copy_process;
		}
	}
#else
	for (i = 0; i < count_calls; i++) {
		movabs_ax = *(uint16_t *)(list_calls[i] - 1);

		if (movabs_ax == 0xb8) {
			c_printk("removing call for copy_process at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = copy_process;
		}
	}
#endif
out:
	free_buffer(&buffer_list_calls_copy_process);
}

void unhook_callsof_exec_binprm(void)
{
	int count_calls;
	int i;
#ifdef BIT_64
	uint16_t movabs_ax;
#else
	uint8_t movabs_ax;
#endif
	uintptr_t *list_calls;

	if (buffer_list_calls_exec_binprm.addr == NULL) {
		c_printk("nothing to unhook for exec_binprm\n");
		goto out;
	}

	count_calls = buffer_list_calls_exec_binprm.size / sizeof(ptr_t);

#ifdef BIT_64
	list_calls = (uintptr_t *)buffer_list_calls_exec_binprm.addr;

	for (i = 0; i < count_calls; i++) {
		movabs_ax = *(uint16_t *)(list_calls[i] - 2);

		if (movabs_ax == 0xb848) {
			c_printk("removing call for exec_binprm at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = exec_binprm;
		}
	}
#else
	for (i = 0; i < count_calls; i++) {
		movabs_ax = *(uint16_t *)(list_calls[i] - 1);

		if (movabs_ax == 0xb8) {
			c_printk("removing call for exec_binprm at 0x%p\n",
				 (ptr_t)list_calls[i]);
			*(ptr_t *)list_calls[i] = exec_binprm;
		}
	}
#endif
out:
	free_buffer(&buffer_list_calls_exec_binprm);
}

void hook_kernel(void)
{
	c_printk("hooking kernel...\n");
	hook_callsof_copy_process();
	hook_callsof_exec_binprm();
	c_printk("hooked kernel\n");
}

void unhook_kernel(void)
{
	c_printk("unhooking kernel...\n");
	unhook_callsof_copy_process();
	unhook_callsof_exec_binprm();
	c_printk("unhooked kernel\n");
}

void communicate_start_thread(void)
{
	if (g_task_communicate != NULL) {
		c_printk(
			"thread to communicate with others processes is already"
			"created?\n");
		return;
	}

	g_task_communicate = kthread_run((ptr_t)communicate_thread_with_tasks,
					 NULL, "communicate_thread");

	if (g_task_communicate) {
		// Increment counter
		get_task_struct(current);

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
}
