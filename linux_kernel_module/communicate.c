#include "communicate.h"

DECLARE_WAIT_QUEUE_HEAD(g_wqh);
/* Thread for communicating */
struct task_struct *g_task_communicate = NULL;
static bool g_task_communicate_stop = false;

struct communicate_header_struct
communicate_read_header(struct task_struct *task, struct vm_area_struct *vma)
{
	struct communicate_header_struct communicate_header;

	if (c_copy_from_user(task, (ptr_t)vma->vm_start, &communicate_header,
			     sizeof(communicate_header))) {
		c_printk("couldn't read communicate header from task %i\n",
			 task->pid);
		communicate_header.state = COMMUNICATE_STATE_READ_HEADER_ERROR;
	}

	return communicate_header;
}

void communicate_reset(struct task_struct *task, struct vm_area_struct *vma)
{
	int state;

	if (vma == NULL) {
		return;
	}

	state = COMMUNICATE_STATE_STOPPED;

	if (c_copy_to_user(task, (ptr_t)vma->vm_start, &state, sizeof(state))) {
		c_printk(
			"couldn't reset communication from task %i with c_copy_to_user\n",
			task->pid);
	}
}

void communicate_check_task(struct task_struct *task)
{
	struct vm_area_struct *vma;

	// No mm?
	if (task->mm == NULL && task->active_mm == NULL) {
		return;
	}

	vma = NULL;

	c_find_vma_from_task(task, &vma, MAGIC_ADDRESS);

	if (vma == NULL) {
		c_printk("did not found vma for communicating from task %i\n",
			 task->pid);
	}
}

void communicate_check_tasks(void)
{
	struct task_struct *task;

	for_each_process (task) {
		communicate_check_task(task);
	}
}

void communicate_with_task(struct task_struct *task)
{
	struct vm_area_struct *vma;

	// No mm?
	if (task->mm == NULL && task->active_mm == NULL) {
		return;
	}

	vma = NULL;

	c_find_vma_from_task(task, &vma, MAGIC_ADDRESS);

	if (vma == NULL) {
		vma = remote_mmap(task, MAGIC_ADDRESS, PROT_COMMUNICATE);

		if (vma != NULL) {
			//	c_printk(
			//	"allocated vma for communicating from task %i\n",
			//	task->pid);

			communicate_reset(task, vma);
		} else {
			c_printk(
				"failed to allocate vma for communicating from task %i\n",
				task->pid);
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

void communicate_thread_with_tasks(bool only_once)
{
	// Wait for wake up
	DECLARE_WAITQUEUE(wq, current);
	add_wait_queue(&g_wqh, &wq);

	c_printk("[communicate] entering into loop\n");

	while (!g_task_communicate_stop) {
		communicate_with_tasks();

		if (only_once) {
			g_task_communicate_stop = true;
		} else {
			msleep(200);
		}
	}

	c_printk("closing thread running\n");

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&g_wqh, &wq);

	c_printk("closed thread to communicate with tasks\n");
	g_task_communicate = NULL;
}

void communicate_start_thread(bool only_once)
{
	if (g_task_communicate != NULL) {
		c_printk(
			"thread to communicate with others processes is already"
			"created?\n");
		return;
	}

	g_task_communicate =
		kthread_run((ptr_t)communicate_thread_with_tasks,
			    (ptr_t)only_once, "communicate_thread");

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
	if (g_task_communicate != NULL && g_task_communicate_stop) {
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
