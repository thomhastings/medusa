#include "text_db.hpp"

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <sstream>
#include <string>
#include <thread>

static std::string wcstr2mbstr(std::wstring const& s)
{
  char *mbs = new char[s.length() + 1];
  std::string result;

  if (wcstombs(mbs, s.c_str(), s.length()) == -1)
    throw std::invalid_argument("convertion failed");

  mbs[s.length()] = '\0';

  result = mbs;

  delete[] mbs;

  return result;
}

TextDatabase::TextDatabase(void)
  : m_DelayLabelModification(false)
{
}

TextDatabase::~TextDatabase(void)
{
}

std::string TextDatabase::GetName(void) const
{
  return "Text";
}

std::string TextDatabase::GetExtension(void) const
{
  return ".mdt";
}

bool TextDatabase::IsCompatible(std::wstring const& rDatabasePath) const
{
  std::ifstream File(wcstr2mbstr(rDatabasePath));
  if (File.is_open() == false)
    return false;
  std::string Line;
  std::getline(File, Line);
  return Line == "# Medusa Text Database\n";
}

bool TextDatabase::Open(std::wstring const& rDatabasePath)
{
  m_TextFile.open(wcstr2mbstr(rDatabasePath), std::ios_base::in | std::ios_base::out);
  return m_TextFile.is_open();
}

bool TextDatabase::Create(std::wstring const& rDatabasePath)
{
  // Returns false if we already have a valid file
  if (m_TextFile.is_open())
    return false;

  // Returns false if the file already exists
  m_TextFile.open(wcstr2mbstr(rDatabasePath), std::ios_base::in | std::ios_base::out);
  if (m_TextFile.is_open() == true)
    return false;

  m_TextFile.open(wcstr2mbstr(rDatabasePath), std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
  return m_TextFile.is_open();
}

bool TextDatabase::Flush(void)
{
  if (!m_TextFile.is_open())
    return false;

  m_TextFile.seekp(0, std::ios::beg);
  m_TextFile.seekg(0, std::ios::beg);

  if (m_TextFile.tellp().seekpos() != 0x0 || m_TextFile.tellg().seekpos() != 0x0)
    return false;

  m_TextFile << "# Medusa Text Database\n";

  // Save binary stream
  /*{
    typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<u8*, 6, 8>> Base64Type;
    std::string Base64Data(Base64Type(...begin), Base64Type(...end));

    m_TextFile << "## BinaryStream\n" << (*itMemArea)->ToString() << "\n" << Base64Data << std::endl;
  }*/

  // Save memory area
  {
    std::lock_guard<std::mutex> Lock(m_MemoryAreaLock);
    m_TextFile << "## MemoryArea\n";
    for (auto itMemArea = std::begin(m_MemoryAreas); itMemArea != std::end(m_MemoryAreas); ++itMemArea)
      m_TextFile << (*itMemArea)->ToString() << "\n";
  }

  // Save label
  {
    std::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
    m_TextFile << "## Label\n";
    for (auto itLabel = std::begin(m_LabelMap.left); itLabel != std::end(m_LabelMap.left); ++itLabel)
      m_TextFile << itLabel->first << " " << itLabel->second.GetName() << "\n";
  }

  // Save cross reference
  {
    std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
    m_TextFile << "## CrossReference\n";
    for (auto itXref = std::begin(m_CrossReferences.GetAllXRefs().left); itXref != std::end(m_CrossReferences.GetAllXRefs().left); ++itXref)
    {
      m_TextFile << itXref->first;
      Address::List From;
      m_CrossReferences.From(itXref->first, From);
      for (auto itAddr = std::begin(From); itAddr != std::end(From); ++itAddr)
        m_TextFile << " \xe2\x86\x90" << *itAddr;
      m_TextFile << "\n";
    }
  }

  // Save multicell
  {
    static const char* MultiCellTypeName[] = { "unknown", "function", "string", "structure", "array" };
    std::lock_guard<std::mutex> Lock(m_MultiCellsLock);
    m_TextFile << "## MultiCell\n";
    // TODO: sanitize MultiCellTypeName
    for (auto itMultiCell = std::begin(m_MultiCells); itMultiCell != std::end(m_MultiCells); ++itMultiCell)
      m_TextFile << itMultiCell->first << ":" << MultiCellTypeName[itMultiCell->second.GetType()] << ":" << itMultiCell->second.GetSize() << "\n";
  }

  {
    static const char* CellTypeName[] = { "unknown", "instruction", "value", "character", "string" };
    std::lock_guard<std::mutex> Lock(m_CellsDataLock);
    m_TextFile << "## Cell\n";
    for (auto itCellData = std::begin(m_CellsData); itCellData != std::end(m_CellsData); ++itCellData)
      m_TextFile
      << itCellData->first
      << ":" << CellTypeName[itCellData->second.GetType()] 
      << ":" << static_cast<int>(itCellData->second.GetSubType())
      << ":" << itCellData->second.GetLength()
      << ":" << itCellData->second.GetFormatStyle()
      << ":" << itCellData->second.GetArchitectureTag()
      << ":" << static_cast<int>(itCellData->second.GetMode())
      << "\n";
  }

  return true;
}

bool TextDatabase::Close(void)
{
  bool Res = Flush();
  m_TextFile.close();
  return Res;
}

bool TextDatabase::AddMemoryArea(MemoryArea* pMemArea)
{
  std::lock_guard<std::mutex> Lock(m_MemoryAreaLock);
  m_MemoryAreas.insert(pMemArea);
  return true;
}

MemoryArea const* TextDatabase::GetMemoryArea(Address const& rAddress) const
{
  std::lock_guard<std::mutex> Lock(m_MemoryAreaLock);
  for (auto itMemArea = std::begin(m_MemoryAreas); itMemArea != std::end(m_MemoryAreas); ++itMemArea)
    if ((*itMemArea)->IsCellPresent(rAddress))
      return *itMemArea;
  return nullptr;
}

bool TextDatabase::AddLabel(Address const& rAddress, Label const& rLabel)
{
  if (HasLabel(rAddress))
    return false;

  std::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
  if (m_DelayLabelModification)
  {
    m_DelayedLabel[rAddress] = std::make_tuple(rLabel, false);
    m_DelayedLabelInverse[rLabel.GetName()] = rAddress;
  }
  else
    m_LabelMap.left.insert(LabelBimapType::left_value_type(rAddress, rLabel));
  return true;
}

bool TextDatabase::RemoveLabel(Address const& rAddress)
{
  boost::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
  auto itLabel = m_LabelMap.left.find(rAddress);
  if (itLabel == std::end(m_LabelMap.left))
    return false;
  if (m_DelayLabelModification)
  {
    m_DelayedLabel[itLabel->first] = std::make_tuple(itLabel->second, true);
    m_DelayedLabelInverse[itLabel->second.GetName()] = rAddress;
  }
  else
    m_LabelMap.left.erase(itLabel);
  return true;
}

bool TextDatabase::HasLabel(Address const& rAddress) const
{
  std::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
  auto itLabel = m_LabelMap.left.find(rAddress);
  auto itLabelEnd = std::end(m_LabelMap.left);

  if (m_DelayLabelModification)
  {
    auto itDelayedLabel = m_DelayedLabel.find(rAddress);
    if (itDelayedLabel != std::end(m_DelayedLabel))
    {
      Address const& rAddr = itDelayedLabel->first;
      bool Remove          = std::get<1>(itDelayedLabel->second);

      if (rAddr == rAddress)
      {
        if (itLabel == itLabelEnd && Remove == false)
          return true;

        if (itLabel != itLabelEnd && Remove == true)
          return false;
      }
    }
  }

  return (itLabel != itLabelEnd);
}

bool TextDatabase::GetLabel(Address const& rAddress, Label& rLabel) const
{
  std::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
  auto itLabel = m_LabelMap.left.find(rAddress);
  if (itLabel == std::end(m_LabelMap.left))
  {
    if (!m_DelayLabelModification)
      return false;

    auto itDelayedLabel = m_DelayedLabel.find(rAddress);
    if (itDelayedLabel != std::end(m_DelayedLabel))
    {
      Address const& rAddr = itDelayedLabel->first;
      Label const& rLbl    = std::get<0>(itDelayedLabel->second);
      bool Remove          = std::get<1>(itDelayedLabel->second);

      if (rAddr == rAddress && Remove == false)
      {
        rLabel = rLbl;
        return true;
      }
    }
    return false;
  }

  rLabel = itLabel->second;
  return true;
}

bool TextDatabase::GetLabelAddress(std::string const& rName, Address& rAddress) const
{
  std::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
  auto itLabel = m_LabelMap.right.find(rName);
  if (itLabel == std::end(m_LabelMap.right))
  {
    if (!m_DelayLabelModification)
      return false;

    auto itDelayedLabelInv = m_DelayedLabelInverse.find(rName);
    if (itDelayedLabelInv != std::end(m_DelayedLabelInverse))
    {
      auto itDelayedLabel = m_DelayedLabel.find(itDelayedLabelInv->second);
      Address const& rAddr = itDelayedLabelInv->second;
      Label const& rLbl    = std::get<0>(itDelayedLabel->second);
      bool Remove          = std::get<1>(itDelayedLabel->second);

      if (rLbl.GetName() == rName && Remove == false)
      {
        rAddress = rAddr;
        return true;
      }
    }
    return false;
  }

  rAddress = itLabel->second;
  return true;
}

// This function is not entirely thread-safe
void TextDatabase::ForEachLabel(std::function<void (Address const& rAddress, Label const& rLabel)> LabelPredicat)
{
  m_DelayLabelModification = true;
  for (auto itLabel = std::begin(m_LabelMap.left); itLabel != std::end(m_LabelMap.left); ++itLabel)
  {
    LabelPredicat(itLabel->first, itLabel->second);
  }
  m_DelayLabelModification = false;
  std::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
  for (auto itDelayedLabel = std::begin(m_DelayedLabel); itDelayedLabel != std::end(m_DelayedLabel); ++itDelayedLabel)
  {
    Address const& rAddr = itDelayedLabel->first;
    Label const& rLbl    = std::get<0>(itDelayedLabel->second);
    bool Remove          = std::get<1>(itDelayedLabel->second);

    if (Remove)
      RemoveLabel(rAddr);
    else
      AddLabel(rAddr, rLbl);
  }
  m_DelayedLabel.clear();
  m_DelayedLabelInverse.clear();
}

bool TextDatabase::AddCrossReference(Address const& rTo, Address const& rFrom)
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
  return m_CrossReferences.AddXRef(rTo, rFrom);
}

bool TextDatabase::RemoveCrossReference(Address const& rFrom)
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
  return m_CrossReferences.RemoveRef(rFrom);
}

bool TextDatabase::RemoveCrossReferences(void)
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
  m_CrossReferences.EraseAll();
  return true;
}

bool TextDatabase::HasCrossReferenceFrom(Address const& rTo) const
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
  return m_CrossReferences.HasXRefFrom(rTo);
}

bool TextDatabase::GetCrossReferenceFrom(Address const& rTo, Address::List& rFromList) const
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
  return m_CrossReferences.From(rTo, rFromList);
}

bool TextDatabase::HasCrossReferenceTo(Address const& rFrom) const
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
  return m_CrossReferences.HasXRefTo(rFrom);
}

bool TextDatabase::GetCrossReferenceTo(Address const& rFrom, Address& rTo) const
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);
  return m_CrossReferences.To(rFrom, rTo);
}

bool TextDatabase::AddMultiCell(Address const& rAddress, MultiCell const& rMultiCell)
{
  std::lock_guard<std::mutex> Lock(m_MemoryAreaLock);
  m_MultiCells[rAddress] = rMultiCell;
  return true;
}

bool TextDatabase::RemoveMultiCell(Address const& rAddress)
{
  std::lock_guard<std::mutex> Lock(m_MemoryAreaLock);

  auto itMultiCell = m_MultiCells.find(rAddress);
  if (itMultiCell == std::end(m_MultiCells))
    return false;
  m_MultiCells.erase(itMultiCell);
  return true;
}

bool TextDatabase::GetMultiCell(Address const& rAddress, MultiCell& rMultiCell) const
{
  std::lock_guard<std::mutex> Lock(m_MemoryAreaLock);
  auto itMultiCell = m_MultiCells.find(rAddress);
  if (itMultiCell == std::end(m_MultiCells))
    return false;
  rMultiCell = itMultiCell->second;
  return true;
}

bool TextDatabase::GetCellData(Address const& rAddress, CellData& rCellData)
{
  std::lock_guard<std::mutex> Lock(m_CellsDataLock);
  auto itCellData = m_CellsData.find(rAddress);
  if (itCellData == std::end(m_CellsData))
    return false;
  rCellData = itCellData->second;
  return true;
}

bool TextDatabase::SetCellData(Address const& rAddress, CellData const& rCellData)
{
  std::lock_guard<std::mutex> Lock(m_CellsDataLock);
  m_CellsData[rAddress] = rCellData;
  return true;
}