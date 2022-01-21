#ifndef KOKABIEL_H
#define KOKABIEL_H

namespace XKLib
{
    /**
     * @brief Manual maps a statically link ELF into a process.
     * TODO:
     * Support dynamic libs.
     */
    class Kokabiel
    {
      public:
        Kokabiel(const std::string& fileName);
        auto inject(Process& process);

      private:
    };
};

#endif
