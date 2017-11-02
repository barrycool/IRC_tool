#include "sirlistwidget.h"

SIRListWidget::SIRListWidget(QWidget *parent):QListWidget(parent)
{
    qDebug() << "SIRListWidget create";
    setAcceptDrops(true);
}

void SIRListWidget::dragEnterEvent( QDragEnterEvent * event )
{
    qDebug() << "dragEnterEvent";

     QListWidget *source = (QListWidget *)((void*)(event->source()));
    if (source && source == this) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void SIRListWidget::dragMoveEvent( QDragMoveEvent * event )
{
    qDebug() << "dragMoveEvent";
    QListWidget *source = (QListWidget *)((void*)(event->source()));
    if (source && source == this) {
        SIRListWidget *lw = (SIRListWidget *)source;
        endPos = event->pos();//得到鼠标移动到的坐标
        QListWidgetItem *startItem = itemAt(startPos);
        QListWidgetItem *endItem = itemAt(endPos);
        int startrow = lw->row(startItem);
        int endrow = lw->row(endItem);
        qDebug() << "startrow:endrow" << startrow << ":" << endrow;
        if(startrow == 0 || endrow == 0)
        {
           qDebug() << "dragMoveEvent:IgnoreAction";
            event->setDropAction(Qt::IgnoreAction);
        }
        else
        {
           qDebug() << "dragMoveEvent:MoveAction";
            event->setDropAction(Qt::MoveAction);
        }
        event->accept();
    }
}

void SIRListWidget::dragLeaveEvent( QDragLeaveEvent * event )
{
    qDebug() << "dragLeaveEvent";
    //to do ,delet the item
    //QListWidget *source = (QListWidget *)((void*)(event->source()));
    //if (source && source == this) {
    //  event->setDropAction(Qt::MoveAction);
    //  event->accept();
    //}
    //else
    //{
    //  event->dra
    //}
}

void SIRListWidget::dropEvent( QDropEvent * event )
{
    QListWidget *source = (QListWidget *)((void*)(event->source()));
    if (source && source == this)
    {
        qDebug() << "dropEvent,this";
        /*
        endPos = event->pos();//得到鼠标移动到的坐标
        QListWidgetItem *itemRow = itemAt(endPos);

        //to do
        event->setDropAction(Qt::MoveAction);
        event->accept();
        */
        QListWidget::dropEvent(event);
        emit internalMoveSiganl();
    }
    else
    {
        const QMimeData* data = event->mimeData();
        if(data->hasText())
        {
            QString texts = data->text() ;
            qDebug() << "dropEvent,other,mimeData()->text():"<< texts;
            //addItem(texts);
        }
        else if(data->hasHtml())
        {
            qDebug() << "dropEvent,other,mimeData()->html():"<< data->html();
        }
        else if(data->hasUrls())
        {
            //event->acceptProposedAction();
            qDebug() << "dropEvent,other,mimeData()->urls:";
        }
        else if(data->hasImage())
        {
             qDebug() << "dropEvent,other,mimeData()->hasImage:";
        }
        else if(data->hasColor())
        {
             qDebug() << "dropEvent,other,mimeData()->hasColor:";
        }
        else
        {
           QListWidgetItem* srcitem = source->currentItem();
           qDebug() << "dropEvent,other,mimeData()->qListWidget:" << srcitem->text();
           endPos = event->pos();
           QListWidgetItem *dstitem = itemAt(endPos);
           int row = this->row(dstitem);
           QStringList textList = srcitem->text().split("     ");;
           QString btnname = textList.at(1);
           emit dropIntoSignal(btnname,row);
        }

        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void SIRListWidget::mousePressEvent( QMouseEvent *event )
{
    qDebug() << "mousePressEvent,event->button():"<<event->button();
    if (event->button() == Qt::LeftButton)
        startPos = event->pos();
    QListWidget::mousePressEvent(event);
}

void SIRListWidget::mouseMoveEvent(QMouseEvent *event)
{
     qDebug() << "mouseMoveEvent";
     QListWidget::mouseMoveEvent(event);
}
