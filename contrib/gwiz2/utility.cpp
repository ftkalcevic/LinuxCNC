#include "stdafx.h"
#include "utility.h"

Utility::Utility()
{
}

bool Utility::ReadImage( QDir &dir, QString sFileName, QPixmap &image )
{
    if ( dir.exists(sFileName) )
    {
        QString path = dir.path() + dir.separator() + sFileName;
        QPixmap imgTemp;
        if ( !imgTemp.load( path ) )
        {
            qDebug() << "Failed to load image '" + path + "'.";
            return false;
        }
        image = imgTemp;
        return true;
    }
    return false;
}



bool Utility::ReadFile( QString sPath, QString &sContents )
{
    QFile file( sPath );
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open file '" + sPath + "'";
        return false;
    }
    
    QTextStream str(&file);
    if ( !str.atEnd() )
    {
        sContents = str.readAll();
        return true;
    }
    return false;
}


QString Utility::ProcessPercents(QString sGCode)
{
    // If the file starts with a percent, we only keep lines between the first and second percent
    QTextStream str( &sGCode );
    QString sLine = str.readLine();
    sLine = sLine.trimmed();
    if ( sLine.length() == 1 && sLine.at(0) == '%' )
    {
        QString sOutput;
        while ( !str.atEnd() )
        {
            QString sLine = str.readLine();
            QString sClean = sLine.trimmed();
            if ( sLine.length() == 1 && sLine.at(0) == '%' )
                break;
            sGCode += sLine + "\n";
        }
    }

    return sGCode;
}

