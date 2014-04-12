#ifndef GWIZLINEEDIT_H
#define GWIZLINEEDIT_H

#include <QLineEdit>

class GWizLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit GWizLineEdit(QWidget *parent = 0);
    
signals:
    void escapePressed();
    
public slots:
    
protected:    
    virtual void keyPressEvent( QKeyEvent * event );
};

#endif // GWIZLINEEDIT_H
