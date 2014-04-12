#include "stdafx.h"
#include "wizard.h"
#include "parameternumeric.h"
#include "parameterlist.h"

const char * const FILE_NAME_DESC = "desc";
const char * const FILE_NAME_CONFIG = "config";
const char * const FILE_NAME_IMAGE = "screen.png";


Wizard::Wizard( QString sPath, QString &sDesc, QString &sOWord )
    : m_sPath( sPath )
    , m_sTreeDesc( sDesc )
    , m_sOWord( sOWord )
{
    m_sOWordFilePath = m_sPath + QDir::separator() + m_sOWord + ".ngc";
    if ( !QFile(m_sOWord).exists() )
    {
        qDebug() << "Failed to find oword '" << m_sOWord << "' implementation file '" << m_sOWordFilePath << "'";
    }
}


Wizard *Wizard::MakeWizard( QDir &dir )
{
    bool bHadDesc = dir.exists(FILE_NAME_DESC);
    bool bHasConfig = dir.exists(FILE_NAME_CONFIG);
    bool bHasImg = dir.exists(FILE_NAME_IMAGE);
    bool bHasVersion = dir.exists("version");
     
    qDebug() << "bHasConfig " << bHasConfig;
    qDebug() << "bHadDesc " << bHadDesc;
    qDebug() << "bHasImg " << bHasImg;
    qDebug() << "bHasVersion " << bHasVersion;

    QString sDesc;
    QString sOWord;
    ReadDesc( dir, sDesc, sOWord );

    QScopedPointer<Wizard> wz( new Wizard( dir.path(), sDesc, sOWord ) );
    if ( !wz->ReadConfig( dir ) )
    {
        return NULL;
    }
    
    QPixmap img;
    if ( Utility::ReadImage( dir, QString(FILE_NAME_IMAGE), img ) )
    {
        wz->m_image = img;
    }
    else
    {
    //    return NULL;
    }
    
    return wz.take();
}

bool Wizard::ReadDesc( QDir &dir, QString &sDesc, QString &sOWord )
{
    if ( dir.exists(FILE_NAME_DESC) )
    {
        QString sContents;
        if ( Utility::ReadFile( dir.path() + QDir::separator() + FILE_NAME_DESC, sContents ))
        {
            QStringList sLines = sContents.split("\n");
            if ( sLines.count() >= 1 )
            {
                QString sLine = sLines[0];
                QStringList sFields = sLine.split("|");
                if ( sFields.count() == 2 )
                {
                    sOWord = sFields[0];
                    QRegExp ex("\\<(.+)\\>");
                    if ( ex.indexIn(sOWord) >= 0 )
                    {
                        sOWord = ex.cap(1);
                    }
                    sOWord = sOWord.toLower();
                    qDebug() << "OWord=" << sOWord;
                    sDesc = sFields[1];
                    return true;
                }
                else
                {
                    qDebug() << "Failed to read 2 fields";
                }
            }
            else
            {
                qDebug() << "Failed to read 1 line";
            }
        }
    }

    sDesc = dir.dirName();
    sOWord = "o<none>";
    return true;
}

bool Wizard::ReadConfig( QDir &dir )
{
    if ( dir.exists(FILE_NAME_CONFIG) )
    {
        QString sContents;
        if ( Utility::ReadFile( dir.path() + QDir::separator() + FILE_NAME_CONFIG, sContents ))
        {
            QStringList sLines = sContents.split("\n",QString::SkipEmptyParts);
            if ( sLines.count() >= 1 )
            {
                bool bFirst = true;
                foreach ( QString sLine, sLines )
                {
                    qDebug() << "sLine='"+sLine+"'";
                    if ( bFirst )
                    {
                        m_sParamDesc = sLine;
                        bFirst = false;
                        qDebug() << "Desc=" << m_sParamDesc;
                    }
                    else
                    {
                        QScopedPointer<Parameter> p;
                        
                        QStringList sFields = sLine.split("|");
                        int idx = 0;
                        QString sType = sFields[idx++];
                        QString sDesc = sFields[idx++];
                        QString sTooltip = sFields[idx++];
                        qDebug() << "sType=" << sType;
                        qDebug() << "sDesc=" << sDesc;
                        qDebug() << "sTooltip=" << sTooltip;
                        QString sDefault;
                        if ( sType == "L")  // Specials for list
                        {
                            QString sDefault("0");
                            QScopedPointer<ParameterList> lp( new ParameterList( sDesc, sTooltip, sDefault ) );
                            
                            while ( idx <= sFields.count() - 2 )
                            {
                                QString sValue = sFields[idx++];
                                QString sName  = sFields[idx++];
                                qDebug() << sValue << "," << sName;
                                lp->AddNameValuePair( sValue, sName );
                                
                            }
                            if ( idx < sFields.count() )
                            {
                                sDefault = sFields[idx++];
                            }
                            else
                            {
                                sDefault = "0";     // Old GWiz config files didn't allow a default list value.
                            }
                            lp->SetDefault( sDefault );
                            p.reset( lp.take() );
                            
                        }
                        else
                        {
                            sDefault = sFields[idx++];
                            bool ok = false;
                            double dDefault = sDefault.toDouble( &ok );
                            if ( !ok )
                            {
                                qDebug() << "Failed to convert default value '"+sDefault+"' to a double - defaulting to 0.0";
                            }
                            p.reset( new ParameterNumeric( sDesc, sTooltip, sType == "U" ? true : false, dDefault ) );
                        }
                        
                        m_vParams.append( p.take() );
                        
                        qDebug() << "sDefault=" << sDefault;
                    }
                }
                return true;
            }
            else
            {
                qDebug() << "Failed to read any lines";
            }
        }
    }

    return false;
}

