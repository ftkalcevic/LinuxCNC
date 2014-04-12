#ifndef LISTPARAMETER_H
#define LISTPARAMETER_H

#include "parameter.h"

class ParameterList : public Parameter
{
    Q_OBJECT
public:
    ParameterList(QString &sDesc, QString &sTooltip, QString &sDefaultValue);
    void SetDefault(QString sDefault);
    void AddNameValuePair(QString &sValue, QString &sName);
    
private:
    QMap<QString,QString> m_list;
    QString m_sDefaultValue;
    
public:
    virtual QWidget *MakeWidget( QWidget *parent, QString &sDefault ) const;
    virtual QString GetValue( QWidget *w ) const;
    virtual QString GetDefault() const { return m_sDefaultValue; }

private slots:
    void cboCurrentIndexChanged(int);
};

#endif // LISTPARAMETER_H
