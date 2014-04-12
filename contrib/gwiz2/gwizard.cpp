#include "stdafx.h"
#include "gwizard.h"
#include "wizlineoword.h"

GWizard::GWizard(QObject *parent) 
    : QObject(parent)
    , m_bIsLathe( false )
{
}


bool GWizard::ParseCommandLine()
{
    const QStringList &args = QApplication::arguments();
    QString sIniFile;
    
    int i = 1;  // start at 1 - skip application name.
    while ( i < args.count() )
    {
        if ( args[i] == "-ini" && i+1 < args.count() )
        {
            i++;
            sIniFile = args[i];
        }
        else
        {
            m_sFilenameOnCmdLine = args[i]; // Just remember the last argument
        }
        i++;
    }
    
    if ( sIniFile.isEmpty() )
    {
        sIniFile = getenv( "INI_FILE_NAME" );
    }
    
    if ( sIniFile.isEmpty() )
    {
        qFatal( "No linuxcnc .ini file specified.  Add it to the command line, eg -ini <path_to_ini>, or set the environment variable, INI_FILE_NAME");
    }
    
    if ( !QFile(sIniFile).exists() )
    {
        qFatal( "Failed to load linuxcnc ini file '%s'.", sIniFile.toLatin1().constData() );
    }

    QSettings settings(sIniFile, QSettings::IniFormat);
    m_sWizardDirs = settings.value( "GWIZARD/WizardDirs" ).toString();
    m_sWizFileDir = settings.value( "GWIZARD/WizFileDir" ).toString();
    m_sWizIniFile = settings.value( "GWIZARD/GWiz2IniFilePath" ).toString();
    m_bIsLathe = settings.value( "DISPLAY/LATHE", 0 ).toInt() == 1;
            
    setenv( "INI_FILE_NAME", sIniFile.toLatin1().constData(), 0 );
    
    return true;
}


bool GWizard::Init()
{
    LoadWizards();
    if ( !m_sFilenameOnCmdLine.isEmpty() )
    {
        return LoadWizFile( m_sFilenameOnCmdLine );
    }
    else
    {
        return NewWizFile();
    }
    return true;
}


bool GWizard::LoadWizards()
{
    // there may be more than one wizard directory
    QStringList sDirs = m_sWizardDirs.split(";");
    
    foreach(QString sDir, sDirs)
    {
        qDebug() << sDir;
        DoLoadWizard( sDir, NULL );
    }
    return true;
}


bool GWizard::DoLoadWizard( QString sDirPath, Wizard *wzParent )
{
    qDebug() << "DoLoadWizard - " << sDirPath;
    QDir dir(sDirPath);
    Wizard *wz = Wizard::MakeWizard( dir );
    if ( wz != NULL )
    {
        if ( wzParent == NULL )
        {
            // Top level
            m_wizards.append( wz );
        }
        else
        {
            wzParent->AddChild( wz );
        }
        
        if ( !wz->GetOWord().isEmpty() )
        {
            if ( m_OWordMap.contains( wz->GetOWord() ) )
            {
                qDebug() << "Duplicate oword wizard - " << wz->GetOWord();
                Q_ASSERT(false);
            }
            m_OWordMap.insert( wz->GetOWord(), wz );
        }
    }
    
    QStringList sSubDirectoryNames = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
    foreach (QString sDirName, sSubDirectoryNames )
    {
        QString sNewPath = sDirPath + QDir::separator() + sDirName;
        DoLoadWizard( sNewPath, wz );
    }
    return true;
}


bool GWizard::LoadWizFile( const QString &sFilename )
{
    m_wf.reset( new WizFile() );
    return m_wf->Load( sFilename, m_OWordMap );    
}

bool GWizard::NewWizFile()
{
    m_wf.reset( new WizFile() );
    return true;
}

static QString RecursivelyLoadNGCs( const QString &sDir )
{
    QString sConcatenatedGCode;
    
    QDir dir(sDir);
    QFileInfoList lstFiles = dir.entryInfoList( QStringList() << "*.ngc" );
    foreach ( QFileInfo fi, lstFiles )
    {
        QString sGCode;
        Utility::ReadFile( fi.canonicalFilePath(), sGCode );
        sGCode = Utility::ProcessPercents( sGCode );
        sConcatenatedGCode += sGCode + "\n";
    }
    
    // And subdirs
    QFileInfoList lstDirs = dir.entryInfoList( QDir::AllDirs | QDir::NoDotAndDotDot );
    foreach ( QFileInfo di, lstDirs )
    {
        QString sGCode = RecursivelyLoadNGCs( di.canonicalFilePath() );
        if ( sGCode.length() > 0 )
            sConcatenatedGCode += sGCode + "\n";
    }
    return sConcatenatedGCode;
}

QString GWizard::MakeGCode( bool bPreview )
{
    // Only load the ngc files once.
    if ( m_sNGCFileContents.isEmpty() )
    {
        // Find all *.ngc files in the wizards tree
        QStringList sDirs = m_sWizardDirs.split(";");
        
        foreach(QString sDir, sDirs)
        {
            m_sNGCFileContents += RecursivelyLoadNGCs( sDir );
        }
    }
    
    // Build the wiz file gcode
    QString sWizGCode;
    int nLine = 1;          // In this situation, wiz lines start from 1.  0 is anything that happens beforehand.
    foreach( WizLine *wl, m_wf->GetWizLines() )
    {
        // if generating for preview, we put comments in the file indicating which wiz line this is
        if ( bPreview )    
        {
            sWizGCode += QString("(" GWIZARD_TRACKING_TOKEN "%1)\n").arg(nLine);
            nLine++;
        }
        sWizGCode += wl->GetLine();
        sWizGCode += "\n";
    }
    sWizGCode = Utility::ProcessPercents(sWizGCode);
    QString sGCodeFile = "%\n" + m_sNGCFileContents + "\n" + sWizGCode + "\n%\n";
    
    return sGCodeFile;
}

