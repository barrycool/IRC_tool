#ifndef SIRLISTWIDGET_H
#define SIRLISTWIDGET_H

#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QDebug>
#include <QDrag>
#include <QPainter>
#include <QEvent>
#include <QDragEnterEvent>
#include <QMimeData>

class SIRListWidget : public QListWidget
{
    Q_OBJECT
public:
    SIRListWidget(QWidget *parent);

protected:
   void mousePressEvent(QMouseEvent *event);
   void mouseMoveEvent(QMouseEvent *event);
   void dragEnterEvent(QDragEnterEvent *event);
   void dragMoveEvent(QDragMoveEvent *event);
   void dropEvent(QDropEvent *event);
   void dragLeaveEvent( QDragLeaveEvent * event );
private:
   QPoint startPos;
   QPoint endPos;

signals:
   dropIntoSignal(QString,int);
   internalMoveSiganl();
   dragLeaveEventSiganl(int);
};

#endif // SIRLISTWIDGET_H
