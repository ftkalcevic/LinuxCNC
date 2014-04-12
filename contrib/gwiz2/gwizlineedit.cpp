#include "stdafx.h"
#include "gwizlineedit.h"

GWizLineEdit::GWizLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    
}


void GWizLineEdit::keyPressEvent( QKeyEvent * event )
{
    if ( event->key() == Qt::Key_Escape )
        emit escapePressed();
    else
        QLineEdit::keyPressEvent(event);
}

