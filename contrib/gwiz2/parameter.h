#ifndef PARAMETER_H
#define PARAMETER_H

class Parameter: public QObject
{
    Q_OBJECT
public:
    Parameter(QString &sDesc, QString &sTooltip);
    
private:
    QString m_sDesc;
    QString m_sTooltip;
    
signals:
    void WidgetChanged( const Parameter & );
    
public:
    const QString &GetDesc() const { return m_sDesc; }
    const QString &GetTooltip() const { return m_sTooltip; }
    virtual QString GetDefault() const = 0;
    
    virtual QWidget *MakeWidget( QWidget *parent, QString &sDefault ) const = 0;
    virtual QString GetValue( QWidget *w ) const = 0;
};

#endif // PARAMETER_H
