#ifndef WIZFILE_H
#define WIZFILE_H

#include "wizline.h"

class WizFile: public QAbstractListModel
{
public:
    WizFile();
    
    bool Load(const QString &sFilename, QMap<QString, Wizard *> &mapOWordMap);
    QVector<WizLine *> &GetWizLines() { return m_FileLines; }
    int rowCount ( const QModelIndex & /*parent*/ = QModelIndex() ) const { return m_FileLines.count(); }
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    void SetDirty(int row = -1 );
    void ClearDirty();
    void AddTextRow(int row, const QString &s);
    WizLine *Cut(int row);
    void MoveUp(int row);
    void MoveDown(int row);
    void AddWizardRow(int row, Wizard *w, const QStringList sParams = QStringList() );
    bool isDirty() const { return m_bDirty; }
    const QString &GetFilename() const { return m_sFilename; }
    bool Save();
    bool SaveAs( const QString &sFilename );
    void AddLine(int row, WizLine *w);
    
private:
    QString m_sFilename;
    QVector<WizLine *> m_FileLines;
    bool m_bDirty;
    
    void Swap(int row1, int row2);
    QString MakeWizFile();
};


#endif // WIZFILE_H
