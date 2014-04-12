#ifndef GWIZLISTVIEW_H
#define GWIZLISTVIEW_H

#include <QListView>

class GWizListView : public QListView
{
    Q_OBJECT
public:
    explicit GWizListView(QWidget *parent = 0);
    
signals:
    void deletePressed();
    void cutPressed();
    void copyPressed();
    void pastePressed();
    void moveUpPressed();
    void moveDownPressed();
    
public slots:
private:
    virtual void keyPressEvent( QKeyEvent * event );
};

#endif // GWIZLISTVIEW_H
