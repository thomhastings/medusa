#include <ios>
#include <iostream>
#include <iomanip>
#include <string>
#include <exception>
#include <stdexcept>
#include <limits>
#include <boost/foreach.hpp>

#include "boost/graph/graphviz.hpp"

#include <medusa/configuration.hpp>
#include <medusa/address.hpp>
#include <medusa/medusa.hpp>
#include <medusa/database.hpp>
#include <medusa/memory_area.hpp>
#include <medusa/log.hpp>
#include <medusa/event_handler.hpp>
#include <medusa/screen.hpp>

MEDUSA_NAMESPACE_USE

  std::ostream& operator<<(std::ostream& out, std::pair<u32, std::string> const& p)
{
  out << p.second;
  return out;
}

class DummyEventHandler : public EventHandler
{
public:
  virtual bool OnDatabaseUpdated(void)
  {
    return true;
  }
};

template<typename Type, typename Container>
class AskFor
{
public:
  Type operator()(Container const& c)
  {
    if (c.empty())
      throw std::runtime_error("Nothing to ask!");

    while (true)
    {
      size_t Count = 0;
      for (typename Container::const_iterator i = c.begin(); i != c.end(); ++i)
      {
        std::cout << Count << " " << (*i)->GetName() << std::endl;
        Count++;
      }
      size_t Input;
      std::cin >> Input;

      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      if (Input < c.size())
        return c[Input];
    }
  }
};

struct AskForConfiguration : public boost::static_visitor<>
{
  AskForConfiguration(Configuration& rCfg) : m_rCfg(rCfg) {}

  Configuration& m_rCfg;

  void operator()(ConfigurationModel::NamedBool const& rBool) const
  {
    std::cout << rBool.GetName() << " " << rBool.GetValue() << std::endl;
    std::cout << "true/false" << std::endl;

    while (true)
    {
      u32 Choose;
      std::string Result;

      std::getline(std::cin, Result, '\n');

      if (Result.empty()) return;

      if (Result == "false" || Result == "true")
      {
        m_rCfg.Set(rBool.GetName(), !!(Result == "true"));
        return;
      }

      std::istringstream iss(Result);
      if (!(iss >> Choose)) continue;

      if (Choose == 0 || Choose == 1)
      {
        m_rCfg.Set(rBool.GetName(), Choose);
        return;
      }
    }
  }

  void operator()(ConfigurationModel::NamedEnum const& rEnum) const
  {
    std::cout << std::dec;
    std::cout << "ENUM TYPE: " << rEnum.GetName() << std::endl;
    for (ConfigurationModel::Enum::const_iterator It = rEnum.GetValue().begin();
      It != rEnum.GetValue().end(); ++It)
    {
      if (It->second == m_rCfg.Get(rEnum.GetName()))
        std::cout << "* ";
      else
        std::cout << "  ";
      std::cout << It->first << ": " << It->second << std::endl;
    }

    while (true)
    {
      u32 Choose;
      std::string Result;

      std::getline(std::cin, Result, '\n');

      if (Result.empty()) return;

      std::istringstream iss(Result);
      if (!(iss >> Choose)) continue;

      for (ConfigurationModel::Enum::const_iterator It = rEnum.GetValue().begin();
        It != rEnum.GetValue().end(); ++It)
        if (It->second == Choose)
        {
          m_rCfg.Set(rEnum.GetName(), Choose);
          return;
        }
    }
  }
};

std::wstring mbstr2wcstr(std::string const& s)
{
  wchar_t *wcs = new wchar_t[s.length() + 1];
  std::wstring result;

  if (mbstowcs(wcs, s.c_str(), s.length()) == -1)
    throw std::invalid_argument("convertion failed");

  wcs[s.length()] = L'\0';

  result = wcs;

  delete[] wcs;

  return result;
}

void DummyLog(std::wstring const & rMsg)
{
  std::wcout << rMsg << std::flush;
}

int main(int argc, char **argv)
{
  std::cout.sync_with_stdio(false);
  std::wcout.sync_with_stdio(false);
  std::string file_path;
  std::string mod_path;
  Log::SetLog(DummyLog);

  try
  {
    if (argc != 2)
      return 0;
    file_path = argv[1];

    std::wstring wfile_path = mbstr2wcstr(file_path);
    std::wstring wmod_path  = L".";

    std::wcout << L"Analyzing the following file: \""         << wfile_path << "\"" << std::endl;
    std::wcout << L"Using the following path for modules: \"" << wmod_path  << "\"" << std::endl;

    Medusa m(wfile_path);

    m.GetDatabase().StartsEventHandling(new DummyEventHandler());
    m.LoadModules(wmod_path);

    if (m.GetSupportedLoaders().empty())
    {
      std::cerr << "Not loader available" << std::endl;
      return EXIT_FAILURE;
    }

    std::cout << "Choose a executable format:" << std::endl;
    AskFor<Loader::VectorSharedPtr::value_type, Loader::VectorSharedPtr> AskForLoader;
    Loader::VectorSharedPtr::value_type pLoader = AskForLoader(m.GetSupportedLoaders());
    std::cout << "Interpreting executable format using \"" << pLoader->GetName() << "\"..." << std::endl;
    pLoader->Map();
    std::cout << std::endl;

    std::cout << "Choose an architecture:" << std::endl;
    AskFor<Architecture::VectorSharedPtr::value_type, Architecture::VectorSharedPtr> AskForArch;
    Architecture::VectorSharedPtr::value_type pArch = pLoader->GetMainArchitecture(m.GetAvailableArchitectures());
    if (!pArch)
      pArch = AskForArch(m.GetAvailableArchitectures());

    std::cout << std::endl;

    ConfigurationModel CfgMdl;
    pArch->FillConfigurationModel(CfgMdl);
    pLoader->Configure(CfgMdl.GetConfiguration());

    //std::cout << "Configuration:" << std::endl;
    //for (ConfigurationModel::ConstIterator It = CfgMdl.Begin(); It != CfgMdl.End(); ++It)
    //  boost::apply_visitor(AskForConfiguration(CfgMdl.GetConfiguration()), *It);

    pArch->UseConfiguration(CfgMdl.GetConfiguration());
    m.RegisterArchitecture(pArch);


    auto cpu_ctxt = pArch->MakeCpuContext();
    auto mem_ctxt = pArch->MakeMemoryContext();
    auto cpu_info = pArch->GetCpuInformation();

    if (cpu_ctxt == nullptr || mem_ctxt == nullptr || cpu_info == nullptr)
      return 1;

    auto cur_addr = pLoader->GetEntryPoint();
    u64 ip = cur_addr.GetOffset();
    u64 sp = 0x2000000 + 0x40000;
    u32 reg_sz = cpu_info->GetSizeOfRegisterInBit(cpu_info->GetRegisterByType(CpuInformation::ProgramPointerRegister)) / 8;
    cpu_ctxt->WriteRegister(cpu_info->GetRegisterByType(CpuInformation::ProgramPointerRegister), &ip, reg_sz);
    cpu_ctxt->WriteRegister(cpu_info->GetRegisterByType(CpuInformation::StackPointerRegister), &sp, reg_sz);
    std::cout << cpu_ctxt->ToString() << std::endl;

    mem_ctxt->MapDatabase(m.GetDatabase());
    mem_ctxt->AllocateMemory(Address(0x2000000), 0x40000, nullptr);
    std::cout << mem_ctxt->ToString() << std::endl;

    auto emus = m.GetEmulators();
    auto get_interp = emus["interpreter"];
    if (get_interp == nullptr) return 1;
    auto interp = get_interp(pArch->GetCpuInformation(), cpu_ctxt, mem_ctxt);

    std::cout << "Disassembling..." << std::endl;
    m.ConfigureEndianness(pArch);
    m.Analyze(pArch, cur_addr);

    u64 last_ip = ip;
    while (true)
    {
      u64 new_ip = 0;
      cpu_ctxt->ReadRegister(cpu_info->GetRegisterByType(CpuInformation::ProgramPointerRegister), &new_ip, reg_sz);
      cur_addr.SetOffset(new_ip);
      auto cur_insn = dynamic_cast<Instruction const*>(m.GetCell(cur_addr));
      if (cur_insn == nullptr) break;
      std::cout << cur_insn->ToString() << std::endl;
      if (cur_insn->GetSemantic().empty()) break;
      interp->Execute(cur_insn->GetSemantic());
      cpu_ctxt->ReadRegister(cpu_info->GetRegisterByType(CpuInformation::ProgramPointerRegister), &new_ip, reg_sz);
      if (last_ip == new_ip)
      {
        new_ip += cur_insn->GetLength();
        cpu_ctxt->WriteRegister(cpu_info->GetRegisterByType(CpuInformation::ProgramPointerRegister), &new_ip, reg_sz);
      }
      last_ip = new_ip;
      std::cout << cpu_ctxt->ToString() << std::endl;
    }

    return 0;

    //auto mem_area = m.GetDatabase().GetMemoryArea(faddr);
    //if (mem_area == nullptr) throw std::runtime_error("Can't get memory area");
    //std::stack<Address> bb;
    //bb.push(faddr);
    //std::map<Address, bool> visited;
    //while (bb.size())
    //{
    //  auto cur_addr = bb.top();
    //  bb.pop();

    //  if (visited[cur_addr] == true)
    //    continue;
    //  visited[cur_addr] = true;

    //  bool disasm = true;
    //  while (disasm)
    //  {
    //    auto insn = static_cast<Instruction*>(m.GetCell(cur_addr));
    //    if (insn == nullptr) throw std::runtime_error("Can't get instruction");
    //    pArch->FormatCell(m.GetDatabase(), mem_area->GetBinaryStream(), cur_addr, *insn);
    //    auto sem = insn->GetSemantic();
    //    if (!sem.empty())
    //    {
    //      std::for_each(std::begin(sem), std::end(sem), [&](Expression const* expr)
    //      {
    //        std::cout << cur_addr.ToString() << ": " << expr->ToString() << std::endl;
    //      });
    //    }
    //    else
    //      std::cout << cur_addr.ToString() << ": " << insn->ToString() << std::endl;

    //    switch (insn->GetOperationType())
    //    {
    //    case Instruction::OpJump | Instruction::OpCond:
    //      {
    //        Address dst_addr;
    //        if (insn->GetOperandReference(m.GetDatabase(), 0, cur_addr, dst_addr))
    //          bb.push(dst_addr);
    //        break;
    //      }
    //    case Instruction::OpRet:
    //      disasm = false;
    //      break;
    //    }

    //    Address dst_addr;
    //    if (insn->GetOperationType() == Instruction::OpJump && insn->GetOperandReference(m.GetDatabase(), 0, cur_addr, dst_addr) == true)
    //      cur_addr = dst_addr;
    //    else
    //      cur_addr += insn->GetLength();
    //  }
    //}

    //ControlFlowGraph cfg;
    //m.BuildControlFlowGraph(faddr, cfg);
    //cfg.Dump("test.gv", m.GetDatabase());
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch (Exception& e)
  {
    std::wcerr << e.What() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
