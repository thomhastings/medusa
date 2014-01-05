#ifndef __MEDUSA_DATABASE_HPP__
#define __MEDUSA_DATABASE_HPP__

#include "medusa/namespace.hpp"
#include "medusa/types.hpp"
#include "medusa/address.hpp"

#include "medusa/binary_stream.hpp"
#include "medusa/configuration.hpp"
#include "medusa/memory_area.hpp"
#include "medusa/multicell.hpp"
#include "medusa/cell.hpp"
#include "medusa/label.hpp"
#include "medusa/xref.hpp"

MEDUSA_NAMESPACE_BEGIN

class Medusa_EXPORT Database
{
public:
  typedef boost::shared_ptr<Database> SharedPtr;
  typedef std::vector<SharedPtr> VectorSharedPtr;

  Database(void);
  virtual ~Database(void);

  virtual std::string GetName(void) const;
  virtual std::string GetExtension(void) const;
  virtual bool IsCompatible(std::wstring const& rDatabasePath) const;

  virtual bool Open(std::wstring const& rFilePath);
  virtual bool Create(std::wstring const& rDatabasePath);
  virtual bool Flush(void);
  virtual bool Close(void);

  // BinaryStream
  //virtual FileBinaryStream const& GetFileBinaryStream(void) const;

  // MemoryArea
  virtual bool AddMemoryArea(MemoryArea* pMemArea) = 0;
  //virtual RemoveMemoryArea // not implemented
  //virtual MoveMemoryArea // not implemented

  virtual MemoryArea const* GetMemoryArea(Address const& rAddress) const = 0;

  // Label
  virtual bool AddLabel(Address const& rAddress, Label const& rLbl) = 0;
  virtual bool RemoveLabel(Address const& rAddress) = 0;

  virtual bool HasLabel(Address const& rAddress) const = 0;
  virtual bool GetLabel(Address const& rAddress, Label& rLabel) const = 0;
  virtual bool GetLabelAddress(std::string const& rName, Address& rAddress) const = 0;
  virtual void ForEachLabel(std::function<void (Address const& rAddress, Label const& rLabel)> LabelPredicat) = 0;

  // CrossRef
  virtual bool AddCrossReference(Address const& rTo, Address const& rFrom) = 0;
  virtual bool RemoveCrossReference(Address const& rFrom) = 0;
  virtual bool RemoveCrossReferences(void) = 0;

  virtual bool HasCrossReferenceFrom(Address const& rTo) const = 0;
  virtual bool GetCrossReferenceFrom(Address const& rTo, Address::List& rFromList) const = 0;

  virtual bool HasCrossReferenceTo(Address const& rFrom) const = 0;
  virtual bool GetCrossReferenceTo(Address const& rFrom, Address& rTo) const = 0;

  // MultiCell
  virtual bool AddMultiCell(Address const& rAddress, MultiCell const& rMultiCell) = 0;
  virtual bool RemoveMultiCell(Address const& rAddress) = 0;

  virtual bool GetMultiCell(Address const& rAddress, MultiCell& rMultiCell) const = 0;

  // Cell (data)
  virtual bool GetCellData(Address const& rAddress, CellData& rCellData) = 0;
  virtual bool SetCellData(Address const& rAddress, CellData const& rCellData) = 0;
};

typedef Database* (*TGetDabatase)(void);

MEDUSA_NAMESPACE_END

#endif // !__MEDUSA_DATABASE_HPP__
