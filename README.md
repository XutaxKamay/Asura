## Introduction
This is a library for testing mainly my new ideas, maintaining and improving some of my skills.

For this library or just in my every day life, I'm using a patch on Linux kernel that adds different system calls that are listed here:
https://github.com/XutaxKamay/mylib/blob/master/src/custom_linux_syscalls.h

## Requirements
- GNU/Linux:
    - meson
    - g++ / clang++
    - CryptoPP

- For Windows you need MinGW with g++

## How to build
Clone the repo `git clone --recurse-submodules https://github.com/XutaxKamay/XKLib`.

Type `meson build;cd build:meson compile` inside the root directory of the repository.

## How to use

- Just include \<repo\>/src in your project and link the a library. (xklib.rel.a or xklib.dbg.a)

### Warning!!!
If you want to use the library, for GNU/Linux you'll likely need this Linux kernel patch and recompile your kernel:
```diff
diff --git a/arch/um/kernel/process.c b/arch/um/kernel/process.c
index 80504680b..a3f0120d0 100644
--- a/arch/um/kernel/process.c
+++ b/arch/um/kernel/process.c
@@ -34,6 +34,25 @@
 #include <registers.h>
 #include <linux/time-internal.h>
 
+__always_inline struct task_struct *get_current(void)
+{
+       struct task_struct *curtsk = current_thread_info()->task;
+
+       if (curtsk->attached_to)
+               return curtsk->attached_to;
+
+       return curtsk;
+}
+
+EXPORT_SYMBOL(get_current);
+
+__always_inline struct task_struct *get_real_current(void)
+{
+       return real_current;
+}
+
+EXPORT_SYMBOL(get_real_current);
+
 /*
  * This is a per-cpu array.  A processor only modifies its entry and it only
  * cares about its entry, so it's OK if another processor is modifying its
diff --git a/arch/x86/entry/syscalls/syscall_32.tbl b/arch/x86/entry/syscalls/syscall_32.tbl
index 331aaf1a7..b80db4ece 100644
--- a/arch/x86/entry/syscalls/syscall_32.tbl
+++ b/arch/x86/entry/syscalls/syscall_32.tbl
@@ -456,3 +456,8 @@
 449    i386    futex_waitv             sys_futex_waitv
 450    i386    set_mempolicy_home_node         sys_set_mempolicy_home_node
 451    i386    pmadv_ksm               sys_pmadv_ksm
+452    i386    rmmap                   sys_rmmap
+453    i386    rmprotect               sys_rmprotect
+454    i386    pkey_rmprotect          sys_pkey_rmprotect
+455    i386    rmunmap                 sys_rmunmap
+456    i386    rclone                  sys_rclone
diff --git a/arch/x86/entry/syscalls/syscall_64.tbl b/arch/x86/entry/syscalls/syscall_64.tbl
index 14902db4c..eae328ab6 100644
--- a/arch/x86/entry/syscalls/syscall_64.tbl
+++ b/arch/x86/entry/syscalls/syscall_64.tbl
@@ -373,6 +373,11 @@
 449    common  futex_waitv             sys_futex_waitv
 450    common  set_mempolicy_home_node sys_set_mempolicy_home_node
 451    common  pmadv_ksm               sys_pmadv_ksm
+452    common  rmmap                   sys_rmmap
+453    common  rmprotect               sys_rmprotect
+454    common  pkey_rmprotect    sys_pkey_rmprotect
+455    common  rmunmap                 sys_rmunmap
+456    common  rclone     sys_rclone
 
 #
 # Due to a historical design error, certain syscalls are numbered differently
diff --git a/arch/x86/include/asm/current.h b/arch/x86/include/asm/current.h
index 3e204e614..c86a3e524 100644
--- a/arch/x86/include/asm/current.h
+++ b/arch/x86/include/asm/current.h
@@ -10,11 +10,9 @@ struct task_struct;
 
 DECLARE_PER_CPU(struct task_struct *, current_task);
 
-static __always_inline struct task_struct *get_current(void)
-{
-       return this_cpu_read_stable(current_task);
-}
+struct task_struct *get_current(void);
 
+#define real_current this_cpu_read_stable(current_task)
 #define current get_current()
 
 #endif /* __ASSEMBLY__ */
diff --git a/arch/x86/include/asm/unistd.h b/arch/x86/include/asm/unistd.h
index 80e9d5206..300865120 100644
--- a/arch/x86/include/asm/unistd.h
+++ b/arch/x86/include/asm/unistd.h
@@ -56,5 +56,5 @@
 # define __ARCH_WANT_SYS_VFORK
 # define __ARCH_WANT_SYS_CLONE
 # define __ARCH_WANT_SYS_CLONE3
-
+# define __ARCH_WANT_SYS_RFUNCS
 #endif /* _ASM_X86_UNISTD_H */
diff --git a/arch/x86/kernel/process.c b/arch/x86/kernel/process.c
index b370767f5..82f6795a3 100644
--- a/arch/x86/kernel/process.c
+++ b/arch/x86/kernel/process.c
@@ -49,6 +49,18 @@
 
 #include "process.h"
 
+__always_inline struct task_struct *get_current(void)
+{
+       struct task_struct *curtsk = this_cpu_read_stable(current_task);
+
+       if (curtsk->attached_to)
+               return curtsk->attached_to;
+
+       return curtsk;
+}
+
+EXPORT_SYMBOL(get_current);
+
 /*
  * per-CPU TSS segments. Threads are completely 'soft' on Linux,
  * no more per-task TSS's. The TSS size is kept cacheline-aligned
diff --git a/fs/exec.c b/fs/exec.c
index bba8fc449..d3671390b 100644
--- a/fs/exec.c
+++ b/fs/exec.c
@@ -812,7 +812,7 @@ int setup_arg_pages(struct linux_binprm *bprm,
        vm_flags |= mm->def_flags;
        vm_flags |= VM_STACK_INCOMPLETE_SETUP;
 
-       ret = mprotect_fixup(vma, &prev, vma->vm_start, vma->vm_end,
+       ret = mprotect_fixup(current, vma, &prev, vma->vm_start, vma->vm_end,
                        vm_flags);
        if (ret)
                goto out_unlock;
diff --git a/include/asm-generic/current.h b/include/asm-generic/current.h
index 3a2e224b9..371dfffd0 100644
--- a/include/asm-generic/current.h
+++ b/include/asm-generic/current.h
@@ -4,7 +4,9 @@
 
 #include <linux/thread_info.h>
 
-#define get_current() (current_thread_info()->task)
+struct task_struct *get_current(void);
+
+#define real_current current_thread_info()->task
 #define current get_current()
 
 #endif /* __ASM_GENERIC_CURRENT_H */
diff --git a/include/linux/mm.h b/include/linux/mm.h
index 4e8ab4ad4..ad5c6b433 100644
--- a/include/linux/mm.h
+++ b/include/linux/mm.h
@@ -1979,7 +1979,7 @@ extern unsigned long move_page_tables(struct vm_area_struct *vma,
 extern unsigned long change_protection(struct vm_area_struct *vma, unsigned long start,
                              unsigned long end, pgprot_t newprot,
                              unsigned long cp_flags);
-extern int mprotect_fixup(struct vm_area_struct *vma,
+extern int mprotect_fixup(struct task_struct *task, struct vm_area_struct *vma,
                          struct vm_area_struct **pprev, unsigned long start,
                          unsigned long end, unsigned long newflags);
 
diff --git a/include/linux/sched.h b/include/linux/sched.h
index cc1fcacac..9eb3d5428 100644
--- a/include/linux/sched.h
+++ b/include/linux/sched.h
@@ -1525,6 +1525,8 @@ struct task_struct {
        struct callback_head            l1d_flush_kill;
 #endif
 
+       struct task_struct*                     attached_to;
+
        /*
         * New fields for task_struct should be added above here, so that
         * they are included in the randomized portion of task_struct.
diff --git a/include/linux/sched/task.h b/include/linux/sched/task.h
index 719c9a6ca..ea9811fb6 100644
--- a/include/linux/sched/task.h
+++ b/include/linux/sched/task.h
@@ -34,6 +34,8 @@ struct kernel_clone_args {
        int io_thread;
        struct cgroup *cgrp;
        struct css_set *cset;
+       unsigned long ip;
+       struct task_struct *tsk_wtd_mm;
 };
 
 /*
diff --git a/include/linux/syscalls.h b/include/linux/syscalls.h
index 82afad91d..0580189ee 100644
--- a/include/linux/syscalls.h
+++ b/include/linux/syscalls.h
@@ -1270,6 +1270,22 @@ asmlinkage long sys_mmap_pgoff(unsigned long addr, unsigned long len,
                        unsigned long fd, unsigned long pgoff);
 asmlinkage long sys_old_mmap(struct mmap_arg_struct __user *arg);
 
+#ifdef __ARCH_WANT_SYS_RFUNCS
+asmlinkage long sys_rmmap(pid_t pid, unsigned long addr, unsigned long len,
+                         unsigned long prot, unsigned long flags,
+                         unsigned long fd, unsigned long pgoff);
+
+asmlinkage long sys_rmprotect(pid_t pid, unsigned long start, size_t len,
+                             unsigned long prot);
+
+asmlinkage long sys_pkey_rmprotect(pid_t pid, unsigned long start, size_t len,
+                                  unsigned long prot, int pkey);
+
+asmlinkage long sys_rmunmap(pid_t pid, unsigned long addr, size_t len);
+
+asmlinkage long sys_rclone(pid_t pid, unsigned long, unsigned long,
+                          unsigned long, unsigned long);
+#endif
 
 /*
  * Not a real system call, but a placeholder for syscalls which are
diff --git a/include/uapi/asm-generic/unistd.h b/include/uapi/asm-generic/unistd.h
index c780129ab..4728bca2a 100644
--- a/include/uapi/asm-generic/unistd.h
+++ b/include/uapi/asm-generic/unistd.h
@@ -889,8 +889,21 @@ __SYSCALL(__NR_set_mempolicy_home_node, sys_set_mempolicy_home_node)
 #define __NR_pmadv_ksm 451
 __SYSCALL(__NR_pmadv_ksm, sys_pmadv_ksm)
 
+#ifdef __ARCH_WANT_SYS_RFUNCS
+#define __NR_rmmap 452
+__SYSCALL(__NR_rmmap, sys_rmmap);
+#define __NR_rmprotect 453
+__SYSCALL(__NR_rmprotect, sys_rmprotect);
+#define __NR_pkey_rmprotect 454
+__SYSCALL(__NR_pkey_rmprotect, sys_pkey_rmprotect);
+#define __NR_rmunmap 455
+__SYSCALL(__NR_rmunmap, sys_rmunmap);
+#define __NR_rclone 456
+__SYSCALL(__NR_rclone, sys_rclone);
+#endif
+
 #undef __NR_syscalls
-#define __NR_syscalls 452
+#define __NR_syscalls 457
 
 /*
  * 32 bit systems traditionally used different
diff --git a/kernel/fork.c b/kernel/fork.c
index cf07934f5..c0d3508ef 100644
--- a/kernel/fork.c
+++ b/kernel/fork.c
@@ -1546,7 +1546,7 @@ static struct mm_struct *dup_mm(struct task_struct *tsk,
        return NULL;
 }
 
-static int copy_mm(unsigned long clone_flags, struct task_struct *tsk)
+static int copy_mm(unsigned long clone_flags, struct task_struct *tsk, struct task_struct* wtd_tsk)
 {
        struct mm_struct *mm, *oldmm;
 
@@ -1565,7 +1565,7 @@ static int copy_mm(unsigned long clone_flags, struct task_struct *tsk)
         *
         * We need to steal a active VM for that..
         */
-       oldmm = current->mm;
+       oldmm = wtd_tsk->mm;
        if (!oldmm)
                return 0;
 
@@ -1576,7 +1576,7 @@ static int copy_mm(unsigned long clone_flags, struct task_struct *tsk)
                mmget(oldmm);
                mm = oldmm;
        } else {
-               mm = dup_mm(tsk, current->mm);
+               mm = dup_mm(tsk, wtd_tsk->mm);
                if (!mm)
                        return -ENOMEM;
        }
@@ -2241,7 +2241,7 @@ static __latent_entropy struct task_struct *copy_process(
        retval = copy_signal(clone_flags, p);
        if (retval)
                goto bad_fork_cleanup_sighand;
-       retval = copy_mm(clone_flags, p);
+       retval = copy_mm(clone_flags, p, args->tsk_wtd_mm ? args->tsk_wtd_mm : current);
        if (retval)
                goto bad_fork_cleanup_signal;
        retval = copy_namespaces(clone_flags, p);
@@ -2677,6 +2677,16 @@ pid_t kernel_clone(struct kernel_clone_args *args)
                task_unlock(p);
        }
 
+#ifdef __ARCH_WANT_SYS_RFUNCS
+       /* set registers before waking up the forked task */
+       if (args->ip)
+#ifdef __arch_um__
+               PT_REGS_IP(task_pt_regs(p)) = args->ip;
+#else
+               task_pt_regs(p)->ip = args->ip;
+#endif
+#endif
+
        wake_up_new_task(p);
 
        /* forking complete and child started to run, tell ptracer */
@@ -2932,6 +2942,51 @@ SYSCALL_DEFINE2(clone3, struct clone_args __user *, uargs, size_t, size)
 }
 #endif
 
+#ifdef __ARCH_WANT_SYS_RFUNCS
+/**
+ * rclone - create a new process from a remote process,
+ *                     with limited options.
+ * Return: On success, a positive PID for the new process.
+ *         On error, a negative errno number.
+ */
+SYSCALL_DEFINE5(rclone, pid_t, pid, unsigned long, clone_flags, unsigned long,
+               ip, unsigned long, stack, unsigned long, stack_size)
+{
+       long result = -EPERM;
+       struct mm_struct *mm;
+
+       struct kernel_clone_args args = {
+               .flags = (lower_32_bits(clone_flags) & ~CSIGNAL),
+               .exit_signal = (lower_32_bits(clone_flags) & CSIGNAL),
+               .ip = ip,
+               .stack = stack,
+               .stack_size = stack_size,
+               .tsk_wtd_mm = find_get_task_by_vpid(pid),
+       };
+
+       if (!clone3_args_valid(&args))
+               return -EINVAL;
+
+       if (!args.tsk_wtd_mm) {
+               return -EPERM;
+       }
+
+       mm = mm_access(args.tsk_wtd_mm, PTRACE_MODE_ATTACH_REALCREDS);
+       if (!mm || IS_ERR(mm)) {
+               goto put_task_struct;
+       }
+
+       result = kernel_clone(&args);
+
+       mmput(mm);
+
+put_task_struct:
+       put_task_struct(args.tsk_wtd_mm);
+
+       return result;
+}
+#endif
+
 void walk_process_tree(struct task_struct *top, proc_visitor visitor, void *data)
 {
        struct task_struct *leader, *parent, *child;
diff --git a/kernel/sys_ni.c b/kernel/sys_ni.c
index dc765f3ef..3e67344ff 100644
--- a/kernel/sys_ni.c
+++ b/kernel/sys_ni.c
@@ -479,3 +479,11 @@ COND_SYSCALL(setuid16);
 
 /* restartable sequence */
 COND_SYSCALL(rseq);
+
+#ifdef __ARCH_WANT_SYS_RFUNCS
+COND_SYSCALL(rmmap);
+COND_SYSCALL(rmprotect);
+COND_SYSCALL(pkey_rmprotect);
+COND_SYSCALL(rmunmap);
+COND_SYSCALL(rclone);
+#endif
diff --git a/mm/mmap.c b/mm/mmap.c
index 313b57d55..56ab54e86 100644
--- a/mm/mmap.c
+++ b/mm/mmap.c
@@ -2871,10 +2871,10 @@ int do_munmap(struct mm_struct *mm, unsigned long start, size_t len,
        return __do_munmap(mm, start, len, uf, false);
 }
 
-static int __vm_munmap(unsigned long start, size_t len, bool downgrade)
+static int __vm_munmap(struct task_struct *task, unsigned long start, size_t len, bool downgrade)
 {
        int ret;
-       struct mm_struct *mm = current->mm;
+       struct mm_struct *mm = task->mm;
        LIST_HEAD(uf);
 
        if (mmap_write_lock_killable(mm))
@@ -2898,14 +2898,14 @@ static int __vm_munmap(unsigned long start, size_t len, bool downgrade)
 
 int vm_munmap(unsigned long start, size_t len)
 {
-       return __vm_munmap(start, len, false);
+       return __vm_munmap(current, start, len, false);
 }
 EXPORT_SYMBOL(vm_munmap);
 
 SYSCALL_DEFINE2(munmap, unsigned long, addr, size_t, len)
 {
        addr = untagged_addr(addr);
-       return __vm_munmap(addr, len, true);
+       return __vm_munmap(current, addr, len, true);
 }
 
 
@@ -2989,6 +2989,80 @@ SYSCALL_DEFINE5(remap_file_pages, unsigned long, start, unsigned long, size,
        return ret;
 }
 
+#ifdef __ARCH_WANT_SYS_RFUNCS
+SYSCALL_DEFINE3(rmunmap, pid_t, pid, unsigned long, addr, size_t, len)
+{
+       struct task_struct *task = NULL;
+       int result = -EPERM;
+       struct mm_struct *mm;
+
+       task = find_get_task_by_vpid(pid);
+       if (!task) {
+               return -EPERM;
+       }
+
+       mm = mm_access(task, PTRACE_MODE_ATTACH_REALCREDS);
+       if (!mm || IS_ERR(mm)) {
+               goto put_task_struct;
+       }
+
+       addr = untagged_addr(addr);
+       result = __vm_munmap(task, addr, len, true);
+
+       mmput(mm);
+
+put_task_struct:
+       put_task_struct(task);
+
+       return result;
+}
+
+struct user_rmmap {
+       pid_t         pid;
+       unsigned long addr;
+       unsigned long len;
+       unsigned long prot;
+       unsigned long flags;
+       unsigned long fd;
+       unsigned long pgoff;
+};
+
+SYSCALL_DEFINE2(rmmap, struct user_rmmap*, uargs, size_t, usize)
+{
+       struct task_struct *task = NULL;
+       unsigned long result = 0;
+       struct mm_struct *mm;
+       int err;
+       struct user_rmmap args;
+
+       err = copy_struct_from_user(&args, sizeof(args), uargs, usize);
+       if (err) {
+               return err;
+       }
+
+       task = find_get_task_by_vpid(args.pid);
+       if (!task) {
+               return -EINVAL;
+       }
+
+       mm = mm_access(task, PTRACE_MODE_ATTACH_REALCREDS);
+       if (!mm || IS_ERR(mm)) {
+               goto put_task_struct;
+       }
+
+       real_current->attached_to = task;
+       result = ksys_mmap_pgoff(args.addr, args.len, args.prot, args.flags, args.fd, args.pgoff);
+       real_current->attached_to = NULL;
+
+       mmput(mm);
+
+put_task_struct:
+       put_task_struct(task);
+
+       return result;
+}
+#endif
+
 /*
  *  this is really a simplified "do_mmap".  it only handles
  *  anonymous maps.  eventually we may be able to do some
diff --git a/mm/mprotect.c b/mm/mprotect.c
index b69ce7a7b..a6f8851c5 100644
--- a/mm/mprotect.c
+++ b/mm/mprotect.c
@@ -9,6 +9,7 @@
  *  (C) Copyright 2002 Red Hat Inc, All Rights Reserved
  */
 
+#include <linux/sched/mm.h>
 #include <linux/pagewalk.h>
 #include <linux/hugetlb.h>
 #include <linux/shm.h>
@@ -417,7 +418,7 @@ static const struct mm_walk_ops prot_none_walk_ops = {
 };
 
 int
-mprotect_fixup(struct vm_area_struct *vma, struct vm_area_struct **pprev,
+mprotect_fixup(struct task_struct *task, struct vm_area_struct *vma, struct vm_area_struct **pprev,
        unsigned long start, unsigned long end, unsigned long newflags)
 {
        struct mm_struct *mm = vma->vm_mm;
@@ -443,7 +444,7 @@ mprotect_fixup(struct vm_area_struct *vma, struct vm_area_struct **pprev,
            (newflags & VM_ACCESS_FLAGS) == 0) {
                pgprot_t new_pgprot = vm_get_page_prot(newflags);
 
-               error = walk_page_range(current->mm, start, end,
+               error = walk_page_range(task->mm, start, end,
                                &prot_none_walk_ops, &new_pgprot);
                if (error)
                        return error;
@@ -530,14 +531,14 @@ mprotect_fixup(struct vm_area_struct *vma, struct vm_area_struct **pprev,
 /*
  * pkey==-1 when doing a legacy mprotect()
  */
-static int do_mprotect_pkey(unsigned long start, size_t len,
+static int do_mprotect_pkey(struct task_struct *task, unsigned long start, size_t len,
                unsigned long prot, int pkey)
 {
        unsigned long nstart, end, tmp, reqprot;
        struct vm_area_struct *vma, *prev;
        int error = -EINVAL;
        const int grows = prot & (PROT_GROWSDOWN|PROT_GROWSUP);
-       const bool rier = (current->personality & READ_IMPLIES_EXEC) &&
+       const bool rier = (task->personality & READ_IMPLIES_EXEC) &&
                                (prot & PROT_READ);
 
        start = untagged_addr(start);
@@ -559,7 +560,7 @@ static int do_mprotect_pkey(unsigned long start, size_t len,
 
        reqprot = prot;
 
-       if (mmap_write_lock_killable(current->mm))
+       if (mmap_write_lock_killable(task->mm))
                return -EINTR;
 
        /*
@@ -567,10 +568,10 @@ static int do_mprotect_pkey(unsigned long start, size_t len,
         * them use it here.
         */
        error = -EINVAL;
-       if ((pkey != -1) && !mm_pkey_is_allocated(current->mm, pkey))
+       if ((pkey != -1) && !mm_pkey_is_allocated(task->mm, pkey))
                goto out;
 
-       vma = find_vma(current->mm, start);
+       vma = find_vma(task->mm, start);
        error = -ENOMEM;
        if (!vma)
                goto out;
@@ -647,7 +648,7 @@ static int do_mprotect_pkey(unsigned long start, size_t len,
                                goto out;
                }
 
-               error = mprotect_fixup(vma, &prev, nstart, tmp, newflags);
+               error = mprotect_fixup(task, vma, &prev, nstart, tmp, newflags);
                if (error)
                        goto out;
 
@@ -666,22 +667,79 @@ static int do_mprotect_pkey(unsigned long start, size_t len,
                prot = reqprot;
        }
 out:
-       mmap_write_unlock(current->mm);
+       mmap_write_unlock(task->mm);
        return error;
 }
 
-SYSCALL_DEFINE3(mprotect, unsigned long, start, size_t, len,
+#ifdef __ARCH_WANT_SYS_RFUNCS
+SYSCALL_DEFINE4(rmprotect, pid_t, pid, unsigned long, start, size_t, len,
                unsigned long, prot)
 {
-       return do_mprotect_pkey(start, len, prot, -1);
+       struct task_struct *task = NULL;
+       int result = -EPERM;
+       struct mm_struct *mm;
+
+       task = find_get_task_by_vpid(pid);
+       if (!task) {
+               return -EPERM;
+       }
+
+       mm = mm_access(task, PTRACE_MODE_ATTACH_REALCREDS);
+       if (!mm || IS_ERR(mm)) {
+               goto put_task_struct;
+       }
+
+       result = do_mprotect_pkey(task, start, len, prot, -1);
+
+       mmput(mm);
+
+put_task_struct:
+       put_task_struct(task);
+
+       return result;
 }
 
 #ifdef CONFIG_ARCH_HAS_PKEYS
+SYSCALL_DEFINE5(pkey_rmprotect, pid_t, pid, unsigned long, start, size_t, len,
+               unsigned long, prot, int, pkey)
+{
+       struct task_struct *task = NULL;
+       int result = -EPERM;
+       struct mm_struct *mm;
+
+       task = find_get_task_by_vpid(pid);
+       if (!task) {
+               return -EPERM;
+       }
 
-SYSCALL_DEFINE4(pkey_mprotect, unsigned long, start, size_t, len,
+       mm = mm_access(task, PTRACE_MODE_ATTACH_REALCREDS);
+       if (!mm || IS_ERR(mm)) {
+               goto put_task_struct;
+       }
+
+       result = do_mprotect_pkey(task, start, len, prot, pkey);
+
+       mmput(mm);
+
+put_task_struct:
+       put_task_struct(task);
+
+       return result;
+}
+#endif
+#endif
+
+SYSCALL_DEFINE3(mprotect, unsigned long, start, size_t, len, unsigned long,
+               prot)
+{
+       return do_mprotect_pkey(current, start, len, prot, -1);
+}
+
+#ifdef CONFIG_ARCH_HAS_PKEYS
+ SYSCALL_DEFINE4(pkey_mprotect, unsigned long, start, size_t, len,
                unsigned long, prot, int, pkey)
 {
-       return do_mprotect_pkey(start, len, prot, pkey);
+       return do_mprotect_pkey(current, start, len, prot, pkey);
 }
 
 SYSCALL_DEFINE2(pkey_alloc, unsigned long, flags, unsigned long, init_val)

```
You might also need to fix the patch to suit your kernel version (changing system calls numbers).
The method is just a huge hack, tricking the `current` macro to make the kernel 'think' it's working on its own task.

Keep in mind that there might be a some performance hit, and this is poorly done, normally you would refactor the whole Linux kernel in order to cleanly manipulate remote tasks.

## What about using ptrace?
ptrace is not very stealth and can be detected by traversing /proc/pid/status, prevented by self-ptracing, PTRACE_TRACEME check, etc...
Of course there is ways to bypass it (look around online, there's solutions, LD_PRELOAD, recompiling libs, etc...)

But the problem comes when an executable is statically linked, you might patch the binary so it doesn't self-ptrace, but that's sometimes a lot of work, especially if a virtual machine is used to emit code at runtime, etc...
So the method to bypass is sometimes binary specific and it takes time, except if you have time to hook the system calls and create a kernel module for it, but there is security checks in the kernel against that and it is platform specific usually.

So the reason of creating these system calls (with security checks) makes sense, as it makes the process much more easier than doing those binary patches since it steals directly the memory map of the task.

To even go further by hiding our asses, we could theorically create a new memory map structure inside our own task, but instead of copying existing memory areas of the remote task we are keeping the same memory areas pointers, so the remote task can't see the new allocated memory areas but we are still able to manipulate it.

The other easier method is to add a special dirty flag to the memory area so it hides a bit the new allocated memory area from the remote task (/proc/pid/maps), but it is clearly not a proper solution.

## Testing & Examples
There is tests and examples being done on my code at test/src and being compiled as xklib_test.rel / xklib_test.dbg
