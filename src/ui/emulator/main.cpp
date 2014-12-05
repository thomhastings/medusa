#include <ios>
#include <iostream>
#include <iomanip>
#include <string>
#include <exception>
#include <stdexcept>
#include <limits>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem/path.hpp>

#include <medusa/configuration.hpp>
#include <medusa/address.hpp>
#include <medusa/medusa.hpp>
#include <medusa/document.hpp>
#include <medusa/memory_area.hpp>
#include <medusa/log.hpp>
#include <medusa/event_handler.hpp>
#include <medusa/disassembly_view.hpp>
#include <medusa/view.hpp>
#include <medusa/module.hpp>
#include <medusa/user_configuration.hpp>
#include <medusa/execution.hpp>

MEDUSA_NAMESPACE_USE

class TextFullDisassemblyView : public FullDisassemblyView
{
public:
  TextFullDisassemblyView(Medusa& rCore, u32 FormatFlags, u32 Width, u32 Height, Address const& rAddress)
  : FullDisassemblyView(rCore, FormatFlags, Width, Height, rAddress)
  {}

  void Print(void)
  {
    std::cout << m_PrintData.GetTexts() << std::endl;
  }

};

std::ostream& operator<<(std::ostream& out, std::pair<u32, std::string> const& p)
{
  out << p.second;
  return out;
}

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
  AskForConfiguration(ConfigurationModel& rCfgMdl) : m_rCfgMdl(rCfgMdl) {}

  ConfigurationModel& m_rCfgMdl;

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
        m_rCfgMdl.SetEnum(rBool.GetName(), !!(Result == "true"));
        return;
      }

      std::istringstream iss(Result);
      if (!(iss >> Choose)) continue;

      if (Choose == 0 || Choose == 1)
      {
        m_rCfgMdl.SetEnum(rBool.GetName(), Choose ? true : false);
        return;
      }
    }
  }

  void operator()(ConfigurationModel::NamedUint8 const& rUint8) const
  {
    std::cout << rUint8.GetName() << " " << static_cast<int>(rUint8.GetValue()) << std::endl;

    while (true)
    {
      u64 Integer;

      std::cin >> Integer;

      if (Integer <= std::pow(2, sizeof(decltype(rUint8.GetValue())) * 8) - 1)
      {
        m_rCfgMdl.SetUint8(rUint8.GetName(), static_cast<u8>(Integer));
        return;
      }
    }
  }

  void operator()(ConfigurationModel::NamedUint16 const& rUint16) const
  {
    std::cout << rUint16.GetName() << " " << static_cast<int>(rUint16.GetValue()) << std::endl;

    while (true)
    {
      u64 Integer;

      std::cin >> Integer;

      if (Integer <= std::pow(2, sizeof(decltype(rUint16.GetValue())) * 8) - 1)
      {
        m_rCfgMdl.SetUint16(rUint16.GetName(), static_cast<u16>(Integer));
        return;
      }
    }
  }

  void operator()(ConfigurationModel::NamedUint32 const& rUint32) const
  {
    std::cout << rUint32.GetName() << " " << static_cast<int>(rUint32.GetValue()) << std::endl;

    while (true)
    {
      u64 Integer;

      std::cin >> Integer;

      if (Integer <= std::pow(2, sizeof(decltype(rUint32.GetValue())) * 8) - 1)
      {
        m_rCfgMdl.SetUint32(rUint32.GetName(), static_cast<u32>(Integer));
        return;
      }
    }
  }

  void operator()(ConfigurationModel::NamedUint64 const& rUint64) const
  {
    std::cout << rUint64.GetName() << " " << static_cast<int>(rUint64.GetValue()) << std::endl;

    while (true)
    {
      u64 Integer;

      std::cin >> Integer;

      if (Integer <= std::pow(2, sizeof(decltype(rUint64.GetValue())) * 8) - 1)
      {
        m_rCfgMdl.SetUint64(rUint64.GetName(), static_cast<u64>(Integer));
        return;
      }
    }
  }

  void operator()(ConfigurationModel::NamedEnum const& rEnum) const
  {
    std::cout << std::dec;
    std::cout << "ENUM TYPE: " << rEnum.GetName() << std::endl;
    for (Configuration::Enum::const_iterator It = rEnum.GetConfigurationValue().begin();
      It != rEnum.GetConfigurationValue().end(); ++It)
    {
      if (It->second == m_rCfgMdl.GetEnum(rEnum.GetName()))
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

      for (Configuration::Enum::const_iterator It = rEnum.GetConfigurationValue().begin();
        It != rEnum.GetConfigurationValue().end(); ++It)
        if (It->second == Choose)
        {
          m_rCfgMdl.SetEnum(rEnum.GetName(), Choose ? true : false);
          return;
        }
    }
  }

  void operator()(ConfigurationModel::NamedString const& rString) const
  {
    std::cout << rString.GetName() << " " << rString.GetValue() << std::endl;
    std::string Result;

    std::getline(std::cin, Result, '\n');
    if (Result.empty())
      return;
    m_rCfgMdl.SetString(rString.GetName(), Result);
  }

  void operator()(ConfigurationModel::NamedPath const& rPath) const
  {
    std::cout << rPath.GetName() << " " << rPath.GetValue() << std::endl;
    std::string Result;

    std::getline(std::cin, Result, '\n');
    if (Result.empty())
      return;
    m_rCfgMdl.SetPath(rPath.GetName(), Result);
  }
};

void TextLog(std::string const & rMsg)
{
  std::cout << rMsg << std::flush;
}

int main(int argc, char **argv)
{
  boost::filesystem::path file_path;
  boost::filesystem::path mod_path;
  boost::filesystem::path db_path;
  Log::SetLog(TextLog);

  UserConfiguration usr_cfg;
  std::string mod_path_opt;
  if (!usr_cfg.GetOption("core.modules_path", mod_path_opt))
    mod_path = ".";
  else
    mod_path = mod_path_opt;

  try
  {
    if (argc != 3)
      return 0;
    file_path = argv[1];
    db_path = argv[2];

    Log::Write("ui_text") << "Analyzing the following file: \""         << file_path.string() << "\"" << LogEnd;
    Log::Write("ui_text") << "Database will be saved to the file: \""   << db_path.string()   << "\"" << LogEnd;
    Log::Write("ui_text") << "Using the following path for modules: \"" << mod_path.string()  << "\"" << LogEnd;

    Medusa m;
    if (!m.NewDocument(
      std::make_shared<FileBinaryStream>(file_path),
      [&](boost::filesystem::path& rDatabasePath, std::list<Medusa::Filter> const& rExtensionFilter)
      {
        rDatabasePath = db_path;
        return true;
      },
      [&](
        BinaryStream::SPType& rspBinStrm,
        Database::SPType& rspDatabase,
        Loader::SPType& rspLoader,
        Architecture::VSPType& rspArchitectures,
        OperatingSystem::SPType& rspOperatingSystem
      )
      {
      auto& mod_mgr = ModuleManager::Instance();

      mod_mgr.UnloadModules();
      mod_mgr.LoadModules(mod_path, *rspBinStrm);

      if (mod_mgr.GetLoaders().empty())
      {
        Log::Write("ui_text") << "Not loader available" << LogEnd;
        return false;
      }

      std::cout << "Choose a executable format:" << std::endl;
      AskFor<Loader::VSPType::value_type, Loader::VSPType> AskForLoader;
      Loader::VSPType::value_type ldr = AskForLoader(mod_mgr.GetLoaders());
      std::cout << "Interpreting executable format using \"" << ldr->GetName() << "\"..." << std::endl;
      std::cout << std::endl;
      rspLoader = ldr;

      auto archs = mod_mgr.GetArchitectures();
      ldr->FilterAndConfigureArchitectures(archs);
      if (archs.empty())
        throw std::runtime_error("no architecture available");

      rspArchitectures = archs;

      auto os = mod_mgr.GetOperatingSystem(ldr, archs.front());
      rspOperatingSystem = os;

      std::cout << std::endl;

      ConfigurationModel CfgMdl;

      std::cout << "Configuration:" << std::endl;
      for (ConfigurationModel::ConstIterator itCfg = CfgMdl.Begin(), itEnd = CfgMdl.End(); itCfg != CfgMdl.End(); ++itCfg)
        boost::apply_visitor(AskForConfiguration(CfgMdl), itCfg->second);

      AskFor<Database::VSPType::value_type, Database::VSPType> AskForDb;
      auto db = AskForDb(mod_mgr.GetDatabases());
      rspDatabase = db;
      return true;
      },
      [](){ std::cout << "Analyzing..." << std::endl; return true; },
      [](){ return true; }))
    throw std::runtime_error("failed to create new document");

    m.WaitForTasks();

    auto& rDoc = m.GetDocument();
    auto ep = rDoc.GetStartAddress();

    auto pCell = rDoc.GetCell(ep);
    auto spArch = ModuleManager::Instance().GetArchitecture(pCell->GetArchitectureTag());
    if (spArch == nullptr)
      throw std::runtime_error("unable to find architecture");

    std::vector<std::string> Args;
    std::vector<std::string> Env;
    Args.push_back(file_path.string());

    Execution exec(m.GetDocument(), spArch, nullptr);
    if (!exec.Initialize(m.GetDocument().GetMode(m.GetDocument().GetStartAddress()), Args, Env, nullptr))
    {
      std::cerr << "Unable to initialize emulator" << std::endl;
      return 0;
    }
    if (!exec.SetEmulator("interpreter"))
    {
      std::cerr << "Unable to set the emulator" << std::endl;
      return 0;
    }

    exec.Execute(m.GetDocument().GetAddressFromLabelName("start"));

    //auto fnApiStub = [](CpuContext* pCpuCtxt, MemoryContext* pMemCtxt)
    //{
    //  std::cout << "API called! Let's gtfo..." << std::endl;
    //  auto RegPc = pCpuCtxt->GetCpuInformation().GetRegisterByType(CpuInformation::ProgramPointerRegister);
    //  auto RegSp = pCpuCtxt->GetCpuInformation().GetRegisterByType(CpuInformation::StackPointerRegister);
    //  auto RegSz = pCpuCtxt->GetCpuInformation().GetSizeOfRegisterInBit(RegPc) / 8;
    //  u64 RegSpValue = 0x0, RetAddr = 0x0;
    //  pCpuCtxt->ReadRegister(RegSp, &RegSpValue, RegSz);
    //  pMemCtxt->ReadMemory(RegSpValue, &RetAddr, RegSz);
    //  RegSpValue += RegSz;
    //  pCpuCtxt->WriteRegister(RegSp, &RegSpValue, RegSz);
    //  pCpuCtxt->WriteRegister(RegPc, &RetAddr, RegSz);
    //};

    //static char const* apis_name[] =
    //{
    //  "kernel32.dll!GetSystemTimeAsFileTime",
    //  "kernel32.dll!GetCurrentThreadId",
    //  "kernel32.dll!GetTickCount64",
    //  "kernel32.dll!QueryPerformanceCounter",
    //  "msvcr110.dll!__crtGetShowWindowMode",
    //  "kernel32.dll!GetCurrentProcessId",
    //  "kernel32.dll!GetTickCount",
    //  "kernel32.dll!Sleep",
    //  "msvcrt.dll!_initterm",
    //  "msvcr110.dll!_initterm_e",
    //  "msvcr110.dll!_initterm",
    //  "msvcr110d.dll!_initterm_e",
    //  "kernel32.dll!GetProcessHeap",
    //  "kernel32.dll!EncodePointer",
    //  "kernel32.dll!InitializeCriticalSectionAndSpinCount",
    //  "kernel32.dll!FlsAlloc",
    //  "kernel32.dll!SetUnhandledExceptionFilter",
    //  "kernel32.dll!LoadLibraryW",
    //  "kernel32.dll!GetProcAddress",
    //  "msvcrt.dll!malloc",
    //  "msvcrt.dll!_lock",
    //  "msvcrt.dll!__dllonexit",
    //  "msvcrt.dll!_unlock",
    //  "msvcrt.dll!rand",
    //  "msvcrt.dll!puts",
    //  "msvcrt.dll!exit",
    //  "kernel32.dll!GetStartupInfoA",
    //  "kernel32.dll!InterlockedCompareExchange",
    //  "kernel32.dll!HeapSetInformation",
    //  "kernel32.dll!FindFirstFileA",
    //  "kernel32.dll!FindNextFileA",
    //  "kernel32.dll!GetStdHandle",
    //  "kernel32.dll!WriteFile",
    //  "kernel32.dll!CreateFileA",
    //  "kernel32.dll!CreateFileMappingA",
    //  "kernel32.dll!MapViewOfFile",
    //  "kernel32.dll!UnmapViewOfFile",
    //  "kernel32.dll!CloseHandle",
    //  "kernel32.dll!GetFileSize",
    //  "kernel32.dll!SetFilePointer",
    //  "kernel32.dll!SetEndOfFile",
    //  nullptr
    //};

    //u64 FakeValue = 0x9999999999999999ULL;
    //for (auto api_name = apis_name; *api_name; ++api_name)
    //{
    //  Address ApiAddr = m.GetDocument().GetAddressFromLabelName(*api_name);
    //  interp->WriteMemory(ApiAddr, &FakeValue, reg_sz);
    //}
    //interp->AddHook(Address(FakeValue), Emulator::HookOnExecute, fnApiStub);
    //interp->AddHook(Address(FakeValue & 0xffffffff), Emulator::HookOnExecute, fnApiStub);

  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch (Exception& e)
  {
    std::cerr << e.What() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
