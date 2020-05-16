#ifndef RE_DEFINITIONS_H
#define RE_DEFINITIONS_H

/**
 * Functions/symbols that the kernel doesn't export by default which is
 * needed by the kernel module
 */

/**
 * This one is found in memory instead
 */
extern spinlock_t* pcss_set_lock;
extern rwlock_t* ptasklist_lock;

int find_css_set_lock(void);
int find_tasklist_lock(void);

#ifdef __arch_um__
void set_current(struct task_struct* task);
#endif

task_t** get_current_task_ptr(void);
void vma_rb_erase(vm_area_t* vma, rb_root_t* root);
struct task_struct* copy_process(struct pid* pid,
                                 int trace,
                                 int node,
                                 struct kernel_clone_args* args);
 void switch_to_extra(struct task_struct *prev,
                                   struct task_struct *next);

#endif
