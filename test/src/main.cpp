#include "pch.h"

#include "test.h"

auto main() -> int
{
    // XKLib::Test::run();

    XKLib::Kokabiel kok("/home/kamay/c++/toinject64.out");

    auto self = XKLib::Process::self();

    kok.inject(self, { "wow" }, { "HOME=/home/kamay" });

    return 0;
}
