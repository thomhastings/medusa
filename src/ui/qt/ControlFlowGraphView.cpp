#include "ControlFlowGraphView.hpp"

ControlFlowGraphView::ControlFlowGraphView(QWidget* pParent)
  : QGraphicsView(pParent)
  , m_IsMoving(false)
  , m_LastPos()
  , m_LastCursor(Qt::ArrowCursor)
{
}

void ControlFlowGraphView::Zoom(qreal Factor, QPointF CenterPt)
{
  setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  scale(Factor, Factor);
  centerOn(CenterPt);
}

void ControlFlowGraphView::wheelEvent(QWheelEvent *event)
{
  if (event->modifiers().testFlag(Qt::ControlModifier))
  {
    int numSteps = event->delta() / 15 / 8;

    if (numSteps != 0)
    {
      qreal sc = pow(1.25, numSteps);
      Zoom(sc, mapToScene(event->pos()));
      event->accept();
    }
  }
  QGraphicsView::wheelEvent(event);
}

void ControlFlowGraphView::mousePressEvent(QMouseEvent *event)
{
  if (m_IsMoving && event->button() == Qt::LeftButton)
  {
    m_LastCursor = cursor();
    m_LastPos    = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
    return;
  }
  QGraphicsView::mousePressEvent(event);
}

void ControlFlowGraphView::mouseReleaseEvent(QMouseEvent *event)
{
  if (m_IsMoving)
  {
    m_IsMoving = false;
    event->accept();
    return;
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void ControlFlowGraphView::mouseMoveEvent(QMouseEvent *event)
{
  if (m_IsMoving)
  {
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->pos().x() - m_LastPos.x()));
    verticalScrollBar()  ->setValue(verticalScrollBar()->value()   - (event->pos().y() - m_LastPos.y()));
    m_LastPos = event->pos();
    event->accept();
    return;
  }
  QGraphicsView::mouseMoveEvent(event);
}

void ControlFlowGraphView::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Space && !m_IsMoving)
  {
    m_IsMoving   = true;
    m_LastCursor = Qt::ArrowCursor;
    setCursor(Qt::OpenHandCursor);
    return;
  }
  QGraphicsView::keyPressEvent(event);
}

void ControlFlowGraphView::keyReleaseEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Space && m_IsMoving)
  {
    setCursor(m_LastCursor);
    m_IsMoving   = false;
    m_LastCursor = Qt::ArrowCursor;
    return;
  }
  QGraphicsView::keyReleaseEvent(event);
}
