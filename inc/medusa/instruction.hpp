#ifndef MEDUSA_INSTRUCTION_HPP
#define MEDUSA_INSTRUCTION_HPP

#include "medusa/namespace.hpp"
#include "medusa/types.hpp"
#include "medusa/export.hpp"
#include "medusa/cell.hpp"
#include "medusa/operand.hpp"
#include "medusa/document.hpp"
#include "medusa/expression.hpp"

#include <cstring>

MEDUSA_NAMESPACE_BEGIN

#define I_NONE 0x0
#define P_NONE 0x0

//! Instruction is a Cell which handles an instruction for any Architecture.
class Medusa_EXPORT Instruction : public Cell
{
public:
  typedef std::shared_ptr<Instruction> SPType;

  enum
  {
    //! The instruction has specific no type
    NoneType        = 0,
    //! The instruction changes the execution flow
    JumpType        = 1 << 1,
    //! The instruction calls a function
    CallType        = 1 << 2,
    //! The instruction returns from a procedure
    ReturnType      = 1 << 3,
    //! The instruction is conditional
    ConditionalType = 1 << 4
  };

  enum
  {
    InvalidOpcode = 0
  };

  enum
  {
    NoPrefix = 0
  };

  /*! Instruction construction
   * \param Name is the name of the instruction, it must not be allocated.
   * \param Opcode is the unique id for a kind of instruction.
   * \param Length is the length of this instruction.
   */
  Instruction(char const* pName = nullptr, u32 Opcode = InvalidOpcode, u16 Length = 0);

  Instruction(CellData::SPType spDna);

  ~Instruction(void);

  char const* GetName(void) const;
  u32         GetOpcode(void) const;
  u32         GetPrefix(void) const;

  u32 GetTestedFlags(void) const;
  u32 GetUpdatedFlags(void) const;
  u32 GetClearedFlags(void) const;
  u32 GetFixedFlags(void) const;

  Expression::List const& GetSemantic(void) const;

  void               AddOperand(Expression::SPType spOprdExpr);
  Expression::SPType GetOperand(u8 OprdNo) const;
  typedef std::function<void (Expression::SPType const& rspOprdExpr)> OperandCallback;
  void               ForEachExpression(OperandCallback OprdCb) const;

  void SetName(char const* pName);
  void SetOpcode(u32 Opcd);
  void SetTestedFlags(u32 Flags);
  void SetUpdatedFlags(u32 Flags);
  void SetClearedFlags(u32 Flags);
  void SetFixedFlags(u32 Flags);
  void SetSemantic(Expression::List const& rExprList);
  void SetSemantic(Expression::SPType spExpr);
  void AddPreSemantic(Expression::SPType spExpr);
  void AddPostSemantic(Expression::SPType spExpr);

  u16& Length(void);
  u32& Prefix(void);

private:
  char const*         m_pName;            /*! This string holds the instruction name ("call", "lsl", ...)         */
  u32                 m_Opcd;             /*! This integer holds the current opcode (ARM_Ldr, GB_Swap, ...)       */
  u32                 m_Prefix;           /*! This integer holds prefix flag (REP, LOCK, ...)                     */
  u32                 m_TestedFlags;      /*! This integer holds flags that are tested by the instruction         */
  u32                 m_UpdatedFlags;     /*! This integer holds flags that could be modified by the instruction  */
  u32                 m_ClearedFlags;     /*! This integer holds flags that are unset by the instruction          */
  u32                 m_FixedFlags;       /*! This integer holds flags that are set by the instruction            */
  Expression::List    m_Expressions;      /*! This list contains semantic for this instruction if not empty       */
  Expression::VSPType m_Operands;         /*! */

private:
  Instruction(Instruction const&);
  Instruction& operator=(Instruction const&);
  };

MEDUSA_NAMESPACE_END

#endif // MEDUSA_INSTRUCTION_HPP
