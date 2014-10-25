#ifndef MEDUSA_EXPRESSION_VISITOR_HPP
#define MEDUSA_EXPRESSION_VISITOR_HPP

#include "medusa/expression.hpp"

MEDUSA_NAMESPACE_BEGIN

class Medusa_EXPORT CloneVisitor : public ExpressionVisitor
{
public:
  virtual Expression::SPType VisitSystem(SystemExpression::SPType spSysExpr);
  virtual Expression::SPType VisitBind(BindExpression::SPType spBindExpr);
  virtual Expression::SPType VisitTernaryCondition(TernaryConditionExpression::SPType spTernExpr);
  virtual Expression::SPType VisitIfElseCondition(IfElseConditionExpression::SPType spIfElseExpr);
  virtual Expression::SPType VisitWhileCondition(WhileConditionExpression::SPType spWhileExpr);
  virtual Expression::SPType VisitAssignment(AssignmentExpression::SPType spAssignExpr);
  virtual Expression::SPType VisitOperation(OperationExpression::SPType spOpExpr);
  virtual Expression::SPType VisitConstant(ConstantExpression::SPType spConstExpr);
  virtual Expression::SPType VisitIdentifier(IdentifierExpression::SPType spIdExpr);
  virtual Expression::SPType VisitTrackedIdentifier(TrackedIdentifierExpression::SPType spTrkIdExpr);
  virtual Expression::SPType VisitMemory(MemoryExpression::SPType spMemExpr);
  virtual Expression::SPType VisitSymbolic(SymbolicExpression::SPType spSymExpr);
};

class Medusa_EXPORT FilterVisitor : public ExpressionVisitor
{
public:
  typedef std::function <Expression::SPType(Expression::SPType spExpr)> Matcher;
  FilterVisitor(Matcher ExprMatcher, size_t NrOfResult = 0) : m_ExprMatcher(ExprMatcher), m_NbrOfResult(NrOfResult) {}

  virtual Expression::SPType VisitSystem(SystemExpression::SPType spSysExpr);
  virtual Expression::SPType VisitBind(BindExpression::SPType spBindExpr);
  virtual Expression::SPType VisitTernaryCondition(TernaryConditionExpression::SPType spTernExpr);
  virtual Expression::SPType VisitIfElseCondition(IfElseConditionExpression::SPType spIfElseExpr);
  virtual Expression::SPType VisitWhileCondition(WhileConditionExpression::SPType spWhileExpr);
  virtual Expression::SPType VisitAssignment(AssignmentExpression::SPType spAssignExpr);
  virtual Expression::SPType VisitOperation(OperationExpression::SPType spOpExpr);
  virtual Expression::SPType VisitConstant(ConstantExpression::SPType spConstExpr);
  virtual Expression::SPType VisitIdentifier(IdentifierExpression::SPType spIdExpr);
  virtual Expression::SPType VisitTrackedIdentifier(TrackedIdentifierExpression::SPType spTrkIdExpr);
  virtual Expression::SPType VisitMemory(MemoryExpression::SPType spMemExpr);
  virtual Expression::SPType VisitSymbolic(SymbolicExpression::SPType spSymExpr);

  Expression::List GetMatchedExpressions(void) { return m_MatchedExprs; }

protected:
  void _Evaluate(Expression::SPType spExpr);
  void _AddExpression(Expression::SPType spExpr);
  bool _IsDone(void) const;

  Matcher m_ExprMatcher;
  size_t m_NbrOfResult;
  Expression::List m_MatchedExprs;
};

class Medusa_EXPORT EvaluateVisitor : public ExpressionVisitor
{
public:
  // LATER: It'd be better to use a context filled by the architecture itself in order to correctly
  // map the PC pointer. On some architecture the PC is set on multiple register (e.g.: x86 cs:offset,
  // z80/gb bank:offset, ...).
  EvaluateVisitor(Document const& rDoc, u32 PcBaseId, u32 PcOffId, Address const& rCurAddr);

  virtual Expression::SPType VisitSystem(SystemExpression::SPType spSysExpr);
  virtual Expression::SPType VisitBind(BindExpression::SPType spBindExpr);
  virtual Expression::SPType VisitTernaryCondition(TernaryConditionExpression::SPType spTernExpr);
  virtual Expression::SPType VisitIfElseCondition(IfElseConditionExpression::SPType spIfElseExpr);
  virtual Expression::SPType VisitWhileCondition(WhileConditionExpression::SPType spWhileExpr);
  virtual Expression::SPType VisitAssignment(AssignmentExpression::SPType spAssignExpr);
  virtual Expression::SPType VisitOperation(OperationExpression::SPType spOpExpr);
  virtual Expression::SPType VisitConstant(ConstantExpression::SPType spConstExpr);
  virtual Expression::SPType VisitIdentifier(IdentifierExpression::SPType spIdExpr);
  virtual Expression::SPType VisitTrackedIdentifier(TrackedIdentifierExpression::SPType spTrkIdExpr);
  virtual Expression::SPType VisitMemory(MemoryExpression::SPType spMemExpr);
  virtual Expression::SPType VisitSymbolic(SymbolicExpression::SPType spSymExpr);

  bool IsSymbolic(void) const { return m_IsSymbolic; }
  Expression::SPType GetResultExpression(void) const { return m_spResExpr; }

protected:
  Document const& m_rDoc;
  u32 m_PcBaseId;
  u32 m_PcOffId;
  Address const& m_rCurAddr;
  bool m_IsSymbolic;
  Expression::SPType m_spResExpr;
};

//! Visit an expression and convert IdentifierExpression to TrackedIdentifierExpression.
class Medusa_EXPORT TrackVisitor : public CloneVisitor
{
public:
  TrackVisitor(Address const& rCurAddr, Track::Context& rCtxt);
  Expression::SPType GetTrackedExpression(void); // NOTE: caller take the ownership

  virtual Expression::SPType VisitAssignment(AssignmentExpression::SPType spAssignExpr);
  virtual Expression::SPType VisitIdentifier(IdentifierExpression::SPType spIdExpr);
  virtual Expression::SPType VisitTrackedIdentifier(TrackedIdentifierExpression::SPType spTrkIdExpr);

private:
  Address m_CurAddr;
  Track::Context& m_rCtxt;
  Expression::SPType m_spTrackedExpr;
  bool m_IsAssigned;
};

//! BackTrackVisitor looks for TrackedIdentifier expression which correponds to both rAddr and Id
//! If the expression contains a track id, GetResult will return true
//! Visit always return nullptr
//! BackTrackContext is updated with the source
class Medusa_EXPORT BackTrackVisitor : public ExpressionVisitor
{
public:
  BackTrackVisitor(Track::BackTrackContext& rBtCtxt)
    : m_rBtCtxt(rBtCtxt)
    , m_IsAssigned(false), m_TrackSource(false), m_Result(false) {}

  bool GetResult(void) const { return m_Result; }

  virtual Expression::SPType VisitAssignment(AssignmentExpression::SPType spAssignExpr);
  virtual Expression::SPType VisitTrackedIdentifier(TrackedIdentifierExpression::SPType spTrkIdExpr);

private:
  Track::BackTrackContext& m_rBtCtxt;
  bool m_IsAssigned;
  bool m_TrackSource;
  bool m_Result;
};

MEDUSA_NAMESPACE_END

#endif // !MEDUSA_EXPRESSION_VISITOR_HPP