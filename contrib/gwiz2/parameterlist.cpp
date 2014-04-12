#include "stdafx.h"
#include "parameterlist.h"

ParameterList::ParameterList(QString &sDesc, QString &sTooltip, QString &sDefaultValue )
    : Parameter( sDesc, sTooltip )
    , m_sDefaultValue(sDefaultValue)
{
}

void ParameterList::SetDefault(QString sDefault )
{
    m_sDefaultValue = sDefault;
}

void ParameterList::AddNameValuePair( QString &sValue, QString &sName )
{
    if ( m_list.contains(sValue) )
    {
        qDebug() << "List already contains value " << sValue;
    }
    else
    {
        m_list.insert( sValue, sName );
    }
}


QWidget *ParameterList::MakeWidget( QWidget *parent, QString &sInitialValue ) const
{
    QComboBox *cbo = new QComboBox( parent );
    cbo->setEditable( false );
    
    QMapIterator<QString, QString> i(m_list);
    while (i.hasNext()) 
    {
        i.next();
        cbo->addItem( i.value(), i.key() );
    }
    
    if ( !sInitialValue.isEmpty() )
        cbo->setCurrentIndex( cbo->findData(sInitialValue) );
    else
        cbo->setCurrentIndex( cbo->findData(m_sDefaultValue) );

    if ( cbo->currentIndex() < 0 )
        cbo->setCurrentIndex(0);
    
    connect( cbo, SIGNAL(currentIndexChanged(int)), this, SLOT(cboCurrentIndexChanged(int)) );
    
    return cbo;
}


void ParameterList::cboCurrentIndexChanged(int)
{
   emit WidgetChanged( *this );
}


QString ParameterList::GetValue( QWidget *w ) const
{
    QString sRet;
    QComboBox *cbo = dynamic_cast<QComboBox *>(w);
    if ( cbo )
    {
        if ( cbo->currentIndex() >= 0 )
        {
            sRet = cbo->itemData( cbo->currentIndex() ).toString();
        }
        else
        {
            sRet = m_sDefaultValue;
        }
        if ( sRet.isNull() )    // If this is our parameter, we must return a non-null string
            sRet = "";
    }
    return sRet;
}
    
    
