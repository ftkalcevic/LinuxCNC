#include "stdafx.h"
#include "parameternumeric.h"
#include <cfloat>

ParameterNumeric::ParameterNumeric( QString &sDesc, QString &sTooltip, bool bUnsigned, double dDefault )
    : Parameter( sDesc, sTooltip )
    , m_bUnsigned( bUnsigned )
    , m_dDefault( dDefault )
{
    
}


QWidget *ParameterNumeric::MakeWidget( QWidget *parent, QString &sDefault ) const
{
    QLineEdit *e = new QLineEdit( parent );
    if ( m_bUnsigned )
        e->setValidator( new QDoubleValidator(0,DBL_MAX,15));
    else
        e->setValidator( new QDoubleValidator());
    
    if ( !sDefault.isEmpty() )
        e->setText( sDefault);
    else
        e->setText( QString::number(m_dDefault));
    
    connect( e, SIGNAL(textChanged(QString)), this, SLOT(editTextChanged(QString)) );
    
    return e;
}


void ParameterNumeric::editTextChanged(QString)
{
    emit WidgetChanged(*this);
}


QString ParameterNumeric::GetValue( QWidget *w ) const
{
    QString sRet;
    QLineEdit *edt = dynamic_cast<QLineEdit *>(w);
    if ( edt )
    {
        QString s = edt->text();
        bool bOk = false;
        double n = s.toDouble(&bOk);
        if ( !bOk )
        {
            n = 0;
        }
        sRet = QString::number(n);
        if ( sRet.isNull() )    // If this is our parameter, we must return a non-null string
            sRet = "0";
    }
    return sRet;
}
