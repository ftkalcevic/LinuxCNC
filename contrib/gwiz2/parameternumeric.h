#ifndef NUMERICPARAMETER_H
#define NUMERICPARAMETER_H

#include "parameter.h"

class ParameterNumeric : public Parameter
{
    Q_OBJECT
public:
    ParameterNumeric(QString &sDesc, QString &sTooltip, bool bUnsigned, double dDefault);

private:    
    bool m_bUnsigned;
    double m_dDefault;
    
public:
    virtual QWidget *MakeWidget( QWidget *parent, QString &sDefault ) const;
    virtual QString GetValue( QWidget *w ) const;
    virtual QString GetDefault() const { return QString::number(m_dDefault); }
private slots:
    void editTextChanged(QString);
};

#endif // NUMERICPARAMETER_H
