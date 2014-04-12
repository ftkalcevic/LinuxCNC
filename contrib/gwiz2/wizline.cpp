#include "stdafx.h"
#include "wizline.h"
#include "wizlinetext.h"
#include "wizlineoword.h"


WizLine::WizLine( const QString &sLine )
    : m_sLine( sLine )
    , m_bDirty( false )
{
}


WizLine *WizLine::MakeWizLine(QString s, QMap<QString, Wizard*> &mapOWordMap )
{
    QRegExp exOWord("o\\<(.*)>\\s*CALL\\s*\\((.*)\\)");
    
    int idx;
    if ( (idx=exOWord.indexIn(s)) >= 0 )
    {
        QString sOWord = exOWord.cap(1);
        QString sComment = exOWord.cap(2);
        sOWord = sOWord.toLower();
        
        if ( mapOWordMap.contains(sOWord) )
        {
            WizLineOWord *w = new WizLineOWord( mapOWordMap[sOWord], s, exOWord.cap(1), exOWord.cap(2) );
    
            idx += exOWord.matchedLength();
            
            QRegExp exParams("\\s*\\[([\\d\\+-\\.]+)\\]");
            while ((idx = exParams.indexIn(s, idx)) != -1) 
            {
                w->AddParam( exParams.cap(1) );
                idx += exParams.matchedLength();
            }        
            return w;
        }
    }
    
    return new WizLineText(s);
}
