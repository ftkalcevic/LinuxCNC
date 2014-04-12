#include "stdafx.h"
#include "gwizlistview.h"

GWizListView::GWizListView(QWidget *parent) :
    QListView(parent)
{
}


void GWizListView::keyPressEvent( QKeyEvent * event )
{
    if ( (event->key() == Qt::Key_Delete && event->modifiers() & Qt::ShiftModifier) ||
         (event->key() == Qt::Key_X && event->modifiers() & Qt::ControlModifier) )
        emit cutPressed();
    else if ( event->key() == Qt::Key_Delete && !event->modifiers() )
        emit deletePressed();
    else if ( (event->key() == Qt::Key_Insert && event->modifiers() & Qt::ControlModifier) ||
              (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) )
        emit copyPressed();
    else if ( (event->key() == Qt::Key_Insert && event->modifiers() & Qt::ShiftModifier) ||
         (event->key() == Qt::Key_V && event->modifiers() & Qt::ControlModifier) )
        emit pastePressed();
    else if ( event->key() == Qt::Key_Up && event->modifiers() & Qt::ControlModifier )
        emit moveUpPressed();
    else if ( event->key() == Qt::Key_Down && event->modifiers() & Qt::ControlModifier )
        emit moveDownPressed();
    else    
        QListView::keyPressEvent(event);
}

