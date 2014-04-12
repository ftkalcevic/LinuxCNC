#ifndef WIZLINE_H
#define WIZLINE_H

#include "wizard.h"

class WizLine: public QObject
{
public:
    WizLine(const QString &sLine);
    
    static WizLine *MakeWizLine(QString s, QMap<QString, Wizard *> &mapOWordMap);

protected:
    virtual QString RecreateLine() { return m_sLine; }
    QString m_sLine;
    bool m_bDirty;

    
public:
    const QString &GetLine() { if ( m_bDirty ) { m_sLine = RecreateLine(); m_bDirty = false; } return m_sLine; }
    void SetLine( const QString &sNew ) { m_sLine = sNew; }
    virtual QVariant data( int /* role */ ) const { return QVariant(); }
    void SetDirty() { m_bDirty = true; }
    virtual WizLine *Clone() const = 0;
};

#endif // WIZLINE_H
