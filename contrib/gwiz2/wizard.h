#ifndef WIZARD_H
#define WIZARD_H

#include "parameter.h"

class Wizard
{
public:
    Wizard( QString sPath, QString &sDesc, QString &sOWord );
    
    static Wizard *MakeWizard(QDir &dir);
    
private:
    QString m_sPath;
    QString m_sParamDesc;
    QString m_sTreeDesc;
    QString m_sOWord;
    QString m_sOWordFilePath;
    QPixmap m_image;
    QVector<Parameter *> m_vParams;
    QVector<Wizard *> m_children;
    
    static bool ReadDesc(QDir &dir, QString &sDesc, QString &sOWord);
    bool ReadConfig(QDir &dir);
    
public:
    const QString &GetOWord() const { return m_sOWord; }
    const QString &GetTreeDesc() const { return m_sTreeDesc; }
    const QString &GetParamDesc() const { return m_sParamDesc; }
    const QVector<Wizard *> &GetChildren() const { return m_children; }
    void AddChild( Wizard *wz ) { m_children.append(wz); }
    const QPixmap &GetImage() const { return m_image; }
    const QVector<Parameter *> GetParameters() const { return m_vParams; }
    const QString &GetOWordFilePath() const { return m_sOWordFilePath; }
};

#endif // WIZARD_H
