#ifndef WIZLINEOWORD_H
#define WIZLINEOWORD_H

#include "wizline.h"


class WizLineOWord : public WizLine
{
public:
    WizLineOWord( const Wizard *w, const QString &sLine, const QString &sOWord, const QString &sComment );
    WizLineOWord( const Wizard *w );
    void AddParam( QString sParam );
    
private:
    WizLineOWord( const WizLineOWord &rhs );
    
    QString m_sOWord;
    QString m_sComment;
    QStringList m_sParams;
    const Wizard * m_oWizard;
    
public:
    const QString &GetOWord() const { return m_sOWord; }
    const Wizard *GetWizard() const { return m_oWizard; }
    QStringList &GetParams() { return m_sParams; }
    virtual QVariant data( int role ) const { if ( role == WizardRole ) return qVariantFromValue((void *)m_oWizard); return QVariant(); }
    virtual WizLine *Clone() const;
protected:
    virtual QString RecreateLine();
};

#endif // WIZLINEOWORD_H
