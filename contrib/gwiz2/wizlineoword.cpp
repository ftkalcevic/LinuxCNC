#include "stdafx.h"
#include "wizlineoword.h"

WizLineOWord::WizLineOWord(const Wizard *w, const QString &sLine, const QString &sOWord, const QString &sComment )
 : WizLine( sLine )
 , m_sOWord( sOWord )
 , m_sComment( sComment )
 , m_oWizard( w )
{
}

WizLineOWord::WizLineOWord(const Wizard *w )
 : WizLine( "" )
 , m_sOWord( w->GetOWord() )
 , m_sComment( w->GetParamDesc() )
 , m_oWizard( w )
{
    m_bDirty = true;    // Force rebuild of m_sLine;
    
    foreach( Parameter *p, w->GetParameters() )
    {
        AddParam( p->GetDefault() );
    }
}

WizLineOWord::WizLineOWord( const WizLineOWord &rhs )
    : WizLine( rhs.m_sLine )
{
    m_sOWord = rhs.m_sOWord;
    m_sComment = rhs.m_sComment;
    m_oWizard = rhs.m_oWizard;    
    m_sParams = rhs.m_sParams;
}

void WizLineOWord::AddParam( QString sParam )
{
    m_sParams.append( sParam );
}


QString WizLineOWord::RecreateLine() 
{
    QString s;
    s = QString("o<") + m_sOWord + "> CALL (" + m_sComment + ")";
    if ( m_sParams.count() > 0 )
    {
        foreach ( QString p, m_sParams )
        {
            s += QString(" [") + p + "]";
        }
    }
    return s;
}


WizLine *WizLineOWord::Clone() const
{
    return new WizLineOWord( *this );
}
