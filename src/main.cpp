#include "pch.h"

#include "kokabiel.h"

#include "test.h"

auto main() -> int
{
    Asura::Test::run();

    /*
        Asura::Kokabiel kok("/home/kamay/c++/toinject64.out");

        auto self = Asura::Process::self();

        Asura::Kokabiel::injection_info_t injection_info;

        auto task = self.createTask(nullptr);
        auto mmap = self.mmap();

        kok.inject<Asura::Process::TASK_STACK_SIZE,
                   Asura::Kokabiel::arch_t::X86>(mmap,
                                                 { "wow" },
                                                 { "HOME=/home/kamay" },
                                                 task,
                                                 injection_info);

        task.run();
        task.wait();

        kok.freeInjection(injection_info);
        task.freeStack();
    */

    return 0;
}
