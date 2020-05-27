#include "main.h"

__switch_to_t original___switch_to = NULL;

/**
 * Patterns that presents:
 * jmp __switch_to
 */
static pattern_result_t pattern_result;

/**
 * This is used to know what tasks are attached to
 */
task_t* task_attached_to      = NULL;
task_t* current_task_switched = NULL;

/**
 * We could rewrite the whole function here instead to optimize the
 * function a bit.
 * But if the kernel changes it might be not a good option..
 * For now we write twice the current_task, but this not a problem,
 * since we're inside the schedule function.
 * There is one problem with it is that spinlocks/mutexes are sometimes on
 * wrong owner during schedule.
 * Hooking finish_task_switch is one option instead.
 */
__visible __notrace_funcgraph task_t* new___switch_to(task_t* prev,
                                                      task_t* next)
{
    task_t* ret;

    ret = original___switch_to(prev, next);

    if (task_attached_to != NULL && current_task_switched == next)
    {
        current_switch_to(task_attached_to, false);
    }

    return ret;
}

int hook___switch_to(void)
{
    char patterns[][64] = { "41 5F 41 5E 41 5D 41 5C 5B 5D E9 ?? ?? ?? "
                            "?? "
                            "0F 1F 40 00" };

    /**
     * We want only 1 call for now
     */
    init_pattern_result(&pattern_result, 0);

    if (!scan_kernel("__switch_to_asm",
                     "ret_from_fork",
                     patterns[0],
                     strlen(patterns[0]),
                     &pattern_result))
    {
        return -1;
    }

    original___switch_to = (ptr_t)kallsyms_lookup_name("__switch_to");

    if (original___switch_to == NULL)
    {
        c_printk_error("couldn't find the original __switch_to\n");
        return -1;
    }

    if (!hook_rel_jmp(pattern_result.addrs[0] + 10,
                      (uintptr_t)new___switch_to))
    {
        c_printk_error("couldn't hook __switch_to\n");
        return -1;
    }

    c_printk_info("hooked __switch_to\n");

    return 0;
}

void unhook___switch_to(void)
{
    /**
     * Shouldn't fail if we reached here
     */

    if (pattern_result.count != 0)
    {
        hook_rel_jmp(pattern_result.addrs[0] + 10,
                     (uintptr_t)original___switch_to);

        free_pattern_result(&pattern_result);

        c_printk_info("unhooked __switch_to\n");
    }
}

void show_ip(struct pt_regs* regs, const char* loglvl)
{
#ifdef CONFIG_X86_32
    printk("%sEIP: %pS\n", loglvl, (void*)regs->ip);
#else
    printk("%sRIP: %04x:%pS\n", loglvl, (int)regs->cs, (void*)regs->ip);
#endif
    show_opcodes(regs, loglvl);
}

void show_opcodes(struct pt_regs* regs, const char* loglvl)
{
#define PROLOGUE_SIZE  42
#define EPILOGUE_SIZE  21
#define OPCODE_BUFSIZE (PROLOGUE_SIZE + 1 + EPILOGUE_SIZE)
    u8 opcodes[OPCODE_BUFSIZE];
    unsigned long prologue = regs->ip - PROLOGUE_SIZE;
    bool bad_ip;

    /*
     * Make sure userspace isn't trying to trick us into dumping kernel
     * memory by pointing the userspace instruction pointer at it.
     */
    bad_ip = user_mode(regs)
             && __chk_range_not_ok(prologue,
                                   OPCODE_BUFSIZE,
                                   TASK_SIZE_MAX);

    if (bad_ip
        || probe_kernel_read(opcodes, (u8*)prologue, OPCODE_BUFSIZE))
    {
        printk("%sCode: Bad RIP value.\n", loglvl);
    }
    else
    {
        printk(
          "%sCode: %" __stringify(
            PROLOGUE_SIZE) "ph <%02x> %" __stringify(EPILOGUE_SIZE) "ph"
                                                                    "\n",
          loglvl,
          opcodes,
          opcodes[PROLOGUE_SIZE],
          opcodes + PROLOGUE_SIZE + 1);
    }
}

void show_iret_regs(struct pt_regs* regs)
{
    show_ip(regs, KERN_DEFAULT);
    printk(KERN_DEFAULT "RSP: %04x:%016lx EFLAGS: %08lx",
           (int)regs->ss,
           regs->sp,
           regs->flags);
}

void __show_regs(struct pt_regs* regs, enum show_regs_mode mode)
{
    unsigned long cr0 = 0L, cr2 = 0L, cr3 = 0L, cr4 = 0L, fs, gs,
                  shadowgs;
    unsigned long d0, d1, d2, d3, d6, d7;
    unsigned int fsindex, gsindex;
    unsigned int ds, es;

    show_iret_regs(regs);

    if (regs->orig_ax != -1)
        pr_cont(" ORIG_RAX: %016lx\n", regs->orig_ax);
    else
        pr_cont("\n");

    printk(KERN_DEFAULT "RAX: %016lx RBX: %016lx RCX: %016lx\n",
           regs->ax,
           regs->bx,
           regs->cx);
    printk(KERN_DEFAULT "RDX: %016lx RSI: %016lx RDI: %016lx\n",
           regs->dx,
           regs->si,
           regs->di);
    printk(KERN_DEFAULT "RBP: %016lx R08: %016lx R09: %016lx\n",
           regs->bp,
           regs->r8,
           regs->r9);
    printk(KERN_DEFAULT "R10: %016lx R11: %016lx R12: %016lx\n",
           regs->r10,
           regs->r11,
           regs->r12);
    printk(KERN_DEFAULT "R13: %016lx R14: %016lx R15: %016lx\n",
           regs->r13,
           regs->r14,
           regs->r15);

    if (mode == SHOW_REGS_SHORT)
        return;

    if (mode == SHOW_REGS_USER)
    {
        rdmsrl(MSR_FS_BASE, fs);
        rdmsrl(MSR_KERNEL_GS_BASE, shadowgs);
        printk(KERN_DEFAULT "FS:  %016lx GS:  %016lx\n", fs, shadowgs);
        return;
    }

    asm("movl %%ds,%0" : "=r"(ds));
    asm("movl %%es,%0" : "=r"(es));
    asm("movl %%fs,%0" : "=r"(fsindex));
    asm("movl %%gs,%0" : "=r"(gsindex));

    rdmsrl(MSR_FS_BASE, fs);
    rdmsrl(MSR_GS_BASE, gs);
    rdmsrl(MSR_KERNEL_GS_BASE, shadowgs);

    cr0 = read_cr0();
    cr2 = read_cr2();
    cr3 = __read_cr3();
    cr4 = __read_cr4();

    printk(KERN_DEFAULT
           "FS:  %016lx(%04x) GS:%016lx(%04x) knlGS:%016lx\n",
           fs,
           fsindex,
           gs,
           gsindex,
           shadowgs);
    printk(KERN_DEFAULT "CS:  %04lx DS: %04x ES: %04x CR0: %016lx\n",
           regs->cs,
           ds,
           es,
           cr0);
    printk(KERN_DEFAULT "CR2: %016lx CR3: %016lx CR4: %016lx\n",
           cr2,
           cr3,
           cr4);

    get_debugreg(d0, 0);
    get_debugreg(d1, 1);
    get_debugreg(d2, 2);
    get_debugreg(d3, 3);
    get_debugreg(d6, 6);
    get_debugreg(d7, 7);

    /* Only print out debug registers if they are in their non-default
     * state. */
    if (!((d0 == 0) && (d1 == 0) && (d2 == 0) && (d3 == 0)
          && (d6 == DR6_RESERVED) && (d7 == 0x400)))
    {
        printk(KERN_DEFAULT "DR0: %016lx DR1: %016lx DR2: %016lx\n",
               d0,
               d1,
               d2);
        printk(KERN_DEFAULT "DR3: %016lx DR6: %016lx DR7: %016lx\n",
               d3,
               d6,
               d7);
    }

    if (boot_cpu_has(X86_FEATURE_OSPKE))
        printk(KERN_DEFAULT "PKRU: %08x\n", read_pkru());
}

void current_switch_to(task_t* task, bool show_regs)
{
    if (show_regs)
        __show_regs(task_pt_regs(current), SHOW_REGS_ALL);

    /**
     * Write new current task
     */
    this_cpu_write(current_task, task);

    if (show_regs)
        __show_regs(task_pt_regs(current), SHOW_REGS_ALL);
}
