#include "communicate.h"

DECLARE_WAIT_QUEUE_HEAD(g_wqh);
static bool g_task_communicate_stop = false;
struct task_struct *g_task_communicate = NULL;
static __put_task_struct_t original_p___put_task_struct = NULL;
static __put_task_struct_t original_p_put_task_struct = NULL;
static uintptr_t p_movabs_inst_put_task_struct = 0;

// syscalls
// static ptr_t old_fork = NULL;

void communicate_alloc_vma(struct task_struct *task)
{
	// Let's check the process who doesn't have our custom vma
	struct vm_area_struct *vma;

	vma = NULL;

	c_find_vma_from_task(task, &vma, MAGIC_ADDRESS);

	if (vma == NULL) {
		vma = remote_mmap(task->pid, MAGIC_ADDRESS, PROT_RWX);

		if (vma != NULL) {
			c_printk(
				"allocated vma for communicating from task %i\n",
				task->pid);
		}
	}
}

void communicate_alloc_vmas(void)
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

		communicate_alloc_vma(task);
	}
}

void communicate_thread_alloc_vmas(void)
{
	DECLARE_WAITQUEUE(wq, current);
	add_wait_queue(&g_wqh, &wq);

	c_printk("[communicate] entering into loop\n");

	while (!g_task_communicate_stop) {
		communicate_alloc_vmas();

		// Sleep for non constant usage.
		msleep(100);
	}

	c_printk("closing thread running\n");

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&g_wqh, &wq);

	c_printk("closed thread to communicate with tasks\n");
}

void communicate_start_thread(void)
{
	if (g_task_communicate != NULL) {
		c_printk(
			"thread to communicate with others processes is already"
			"created?\n");
		return;
	}

	g_task_communicate = kthread_run((ptr_t)communicate_thread_alloc_vmas,
					 NULL, "communicate_thread");

	if (g_task_communicate) {
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
}

/*
asmlinkage pid_t syscall_fork(void)
{
	struct task_struct *child_task;
	pid_t child_pid;

	child_pid = ((pid_t(*)(void))(old_fork))();

	communicate_alloc_vma(current);

	child_task = pid_task(find_vpid(child_pid), PIDTYPE_PID);

	if (child_task != NULL)
		communicate_alloc_vma(child_task);

	return child_pid;
}

void hook_syscalls(void)
{
	ptr_t *sys_call_table;
	uintptr_t aligned_address;
	pte_t *pte;
	pteval_t pte_oldval;

	sys_call_table = find_sys_call_table();

	if (sys_call_table == NULL) {
		c_printk("couldn't find system call table\n");
		return;
	}

	old_fork = sys_call_table[__NR_fork];

	aligned_address = align_address((uintptr_t)old_fork, PAGE_SIZE);

	pte = get_pte(aligned_address);

	if (pte != NULL) {
		pte_oldval = pte->pte;
		pte->pte |= _PAGE_RW;

		sys_call_table[__NR_fork] = (ptr_t)syscall_fork;

		pte->pte = pte_oldval;
	}
}

void unhook_syscalls(void)
{
	ptr_t *sys_call_table;
	uintptr_t aligned_address;
	pte_t *pte;
	pteval_t pte_oldval;

	sys_call_table = find_sys_call_table();

	if (sys_call_table == NULL) {
		c_printk("couldn't find system call table\n");
		return;
	}

	aligned_address = align_address((uintptr_t)old_fork, PAGE_SIZE);

	pte = get_pte(aligned_address);

	if (pte != NULL) {
		pte_oldval = pte->pte;
		pte->pte |= _PAGE_RW;

		sys_call_table[__NR_fork] = (ptr_t)old_fork;

		pte->pte = pte_oldval;
	}
}
*/

/* Theorically it should be fine to do it this way if this function is called
only in a single thread */
static void new___put_task_struct(struct task_struct *task)
{
	original_p___put_task_struct(task);

	communicate_alloc_vma(task);
}

void hook__put_task_struct(void)
{
	struct buffer_struct buffer_scan;
	char temp_pattern[256];
	uintptr_t temp_address;
	int ret;

	init_buffer(&buffer_scan);

	if (original_p___put_task_struct == NULL)
		original_p___put_task_struct =
			(__put_task_struct_t)kallsyms_lookup_name(
				"__put_task_struct");

	if (original_p___put_task_struct == NULL) {
		c_printk("can't find __put_task_struct\n");
		return;
	}

	temp_address = (uintptr_t)original_p___put_task_struct;

	if (original_p_put_task_struct == NULL)
		original_p_put_task_struct =
			(__put_task_struct_t)kallsyms_lookup_name(
				"put_task_struct");

	if (original_p_put_task_struct == NULL) {
		c_printk("can't find put_task_struct\n");
		return;
	}

	ret = convert_to_hexstring((uint8_t *)&temp_address, sizeof(ptr_t),
				   temp_pattern, sizeof(temp_pattern));

	// Reswap.
	if (!ret) {
		c_printk(
			"couldn't convert __put_task_struct(0x%p) address to hexstring\n",
			original_p___put_task_struct);
		goto out;
	}

	// Scan for movabs, __put_task_struct
	ret = scan_pattern((uintptr_t)original_p_put_task_struct,
			   (uintptr_t)original_p_put_task_struct + 128,
			   temp_pattern, strlen(temp_pattern) - 1,
			   &buffer_scan);

	if (!ret) {
		c_printk("can't find movabs, %p inside put_task_struct\n",
			 original_p_put_task_struct);
		goto out;
	}

	// Read first address.
	p_movabs_inst_put_task_struct = ((uintptr_t *)buffer_scan.addr)[0];

	c_printk("replacing movabs 0x%p to movabs 0x%p on __put_task_struct\n",
		 *(ptr_t *)p_movabs_inst_put_task_struct, (ptr_t)temp_address);

	temp_address = (uintptr_t)new___put_task_struct;
	memcpy((ptr_t)p_movabs_inst_put_task_struct, &temp_address,
	       sizeof(ptr_t));

	c_printk("hooked __put_task_struct\n");

out:
	free_buffer(&buffer_scan);
}

void unhook__put_task_stuct(void)
{
	uintptr_t temp_address;
	c_printk("unhooking __put_task_struct\n");

	memcpy((ptr_t)p_movabs_inst_put_task_struct, &temp_address,
	       sizeof(ptr_t));

	temp_address = (uintptr_t)original_p_put_task_struct;
	memcpy((ptr_t)p_movabs_inst_put_task_struct, &temp_address,
	       sizeof(ptr_t));

	c_printk("unhooked __put_task_struct\n");
}

void hook_kernel(void)
{
	hook__put_task_struct();
}

void unhook_kernel(void)
{
	unhook__put_task_stuct();
}

void __put_task_struct(struct task_struct *task)
{
	static __put_task_struct_t p__put_task_struct = NULL;

	if (p__put_task_struct == NULL) {
		p__put_task_struct = (__put_task_struct_t)kallsyms_lookup_name(
			"__put_task_struct");
	}

	if (p__put_task_struct == NULL) {
		c_printk("couldn't find __put_task_struct\n");
		return;
	}

	p__put_task_struct(task);
}
