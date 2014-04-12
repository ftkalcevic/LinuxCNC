#include "wizlinetext.h"

WizLineText::WizLineText( const QString &sLine )
    : WizLine( sLine )
{
}


WizLine *WizLineText::Clone() const
{
    return new WizLineText( m_sLine );
}
