#ifndef GWIZARD_H
#define GWIZARD_H

#include <QObject>
#include "wizard.h"
#include "wizfile.h"

class GWizard : public QObject
{
    Q_OBJECT
public:
    explicit GWizard(QObject *parent = 0);
    
    bool ParseCommandLine();
    bool Init();
signals:
    
public slots:

private:
    QString m_sWizardDirs;
    QString m_sWizFileDir;
    QString m_sWizIniFile;
    QString m_sFilenameOnCmdLine;
    QVector<Wizard*> m_wizards;
    QMap<QString, Wizard*> m_OWordMap;
    QScopedPointer<WizFile> m_wf;
    QString m_sNGCFileContents;
    bool m_bIsLathe;

    bool LoadWizards();
    bool DoLoadWizard(QString sDirPath, Wizard *wz);
    
public:
    QVector<Wizard*> &Wizards() { return m_wizards; }
    const QString &GetWizFileDirectory() const { return m_sWizFileDir; }
    const QString &GetWizIniFile() const { return m_sWizIniFile; }
    
    bool LoadWizFile(const QString &sFilename);
    bool NewWizFile();
    WizFile *GetWizFile() { return m_wf.data(); }
    QMap<QString, Wizard*>  &GetOWordMap() { return m_OWordMap; }
    QString MakeGCode(bool bPreview);
    bool isLathe() const { return m_bIsLathe; }
};

#endif // GWIZARD_H
