#ifndef ARCH_X86_HELPER_HPP
#define ARCH_X86_HELPER_HPP

#include <medusa/types.hpp>

namespace x86
{
  using medusa::u8;

  class ModRM
  {
  public:
    ModRM(u8 ModRm) : m_ModRm(ModRm) {                          };
    u8 Mod(void) const               { return m_ModRm >> 6;     };
    u8 Reg(void) const               { return m_ModRm >> 3 & 7; };
    u8 Rm(void)  const               { return m_ModRm & 7;      };

  private:
    u8 m_ModRm;
  };

  class Sib
  {
  public:
    Sib(u8 Sib) : m_Sib(Sib) {                        };
    u8 Scale(void) const     { return m_Sib >> 6;     };
    u8 Index(void) const     { return m_Sib >> 3 & 7; };
    u8 Base(void)  const     { return m_Sib & 7;      };

  private:
    u8 m_Sib;
  };
}

#endif // !ARCH_X86_HELPER_HPP
