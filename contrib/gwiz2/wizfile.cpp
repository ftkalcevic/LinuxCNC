#include "stdafx.h"
#include "wizfile.h"
#include "wizline.h"
#include "wizlinetext.h"
#include "wizlineoword.h"

WizFile::WizFile()
    : m_bDirty( false )
{
}

bool WizFile::Load(const QString &sFilename, QMap<QString, Wizard*> &mapOWordMap )
{
    m_sFilename = sFilename;
    
    QFile file( m_sFilename );
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open file '" + m_sFilename + "'";
        return false;
    }
    
    QTextStream str(&file);
    while ( !str.atEnd() )
    {
        QString sLine = str.readLine();
        WizLine *w = WizLine::MakeWizLine(sLine, mapOWordMap );
        m_FileLines.append(w);
    }

    return true;
}

QString WizFile::MakeWizFile()
{
    QString s;
    foreach ( WizLine *wl, m_FileLines)
    {
        s.append( wl->GetLine() );
        s.append( "\n" );
    }
    return s;
}

bool WizFile::Save()
{
    QString s = MakeWizFile();
    
    QFile file( m_sFilename );
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        qDebug() << "Failed to open file '" + m_sFilename + "'";
        return false;
    }
    
    QTextStream str(&file);
    str << s;

    file.close();
    
    ClearDirty();
    return true;
}

bool WizFile::SaveAs( const QString &sFilename )
{
    m_sFilename = sFilename;
    return Save();
}

void WizFile::AddLine( int row, WizLine *w )
{
    beginInsertRows( QModelIndex(),row, row);
    m_FileLines.insert(row,w);
    endInsertRows();
    SetDirty();
}

void WizFile::AddTextRow( int row, const QString &s )
{
    WizLine *w = new WizLineText( s );
    AddLine( row, w );
}

void WizFile::AddWizardRow( int row, Wizard *w, const QStringList sParams )
{
    WizLineOWord *wl = new WizLineOWord( w );
    if ( !sParams.isEmpty() )
        wl->GetParams() = sParams;
    AddLine( row, wl );
}

void WizFile::Swap( int row1, int row2 )
{
    beginMoveRows( QModelIndex(), row1, row1, QModelIndex(), row2 );
    WizLine *w = m_FileLines[row1];
    m_FileLines[row1] = m_FileLines[row2];
    m_FileLines[row2] = w;
    endMoveRows();
    SetDirty();
}

void WizFile::MoveUp( int row )
{
    Swap( row, row - 1 );
}

void WizFile::MoveDown( int row )
{
    Swap( row + 1, row );
}

WizLine *WizFile::Cut( int row )
{
    WizLine *w = m_FileLines[row];
    beginRemoveRows( QModelIndex(),row, row);
    m_FileLines.remove(row);
    endRemoveRows();
    SetDirty();
    return w;
}

QVariant WizFile::data ( const QModelIndex & index, int role ) const
{
    if ( role == Qt::DisplayRole )    
    {
        return QVariant( m_FileLines[ index.row() ]->GetLine() );
    }
    else if ( role == WizardRole )
    {
        WizLine *l = m_FileLines[index.row()];
        return l->data( role );
    }
    else if ( role == WizLineRole )
    {
        WizLine *l = m_FileLines[index.row()];
        return qVariantFromValue((void *)l);
    }
    return QVariant();
}

void WizFile::ClearDirty()
{
    m_bDirty = false;
}

void WizFile::SetDirty(int row)
{
    m_bDirty = true;
    if ( row >= 0 )
    {
        m_FileLines[row]->SetDirty();
        emit dataChanged( index(row), index(row+1) );
    }
}

