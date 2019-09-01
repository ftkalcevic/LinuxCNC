#include "stdafx.h"
#include "hersheyfont.h"

#include <GL/gl.h>
//#include <GL/glut.h>

struct HersheyPoint
{
    int x;
    int y;
    
    HersheyPoint(){}
    HersheyPoint( int _x, int _y ) : x(_x), y(_y) {}
};

struct HersheyChar
{
    int m_left, m_right;
    int m_nWidth;
    QVector< QVector<HersheyPoint> > m_Lines;
};

class HersheyCharacterMap
{
public:
    QMap<int,int> m_mapMap;
    
    HersheyCharacterMap()
    {
    }
    void LoadMap( const QString &sFilePath )
    {
        QFile file(sFilePath);
        file.open( QFile::ReadOnly );
        QTextStream str(&file);
        int nCharCode = 32;
        while ( !str.atEnd() )
        {
            QString sLine = str.readLine();
            QStringList lstIndexes = sLine.split(QChar(' '), QString::SkipEmptyParts );
            
            foreach ( QString s, lstIndexes )
            {
                if ( s.contains(QChar('-') ) )
                {
                    QStringList lstRange = s.split(QChar('-'), QString::SkipEmptyParts );
                    Q_ASSERT(lstRange.length() == 2 );
                     
                    bool bOk = false;
                    int index1 = lstRange[0].toInt(&bOk);
                    Q_ASSERT(bOk);
                    bOk = false;
                    int index2 = lstRange[1].toInt(&bOk);
                    Q_ASSERT(bOk);
                    for ( int i = index1; i <= index2; i++, nCharCode++ )
                    {
                        m_mapMap.insert( nCharCode, i );
                    }
                }
                else 
                {
                    bool bOk = false;
                    int index = s.toInt(&bOk);
                    Q_ASSERT(bOk);
                    m_mapMap.insert( nCharCode, index );
                    nCharCode++;
                }
            }
        }
    }
};

class HersheyFontBase
{
public:
    QMap< int, HersheyChar> m_mapCharacterSet;
    
    HersheyFontBase()
    {
        LoadFullFontSet(g_pAppSettings->FontPath());
    }
    void LoadFullFontSet( const QString &sFilePath )
    {
        QFile file( sFilePath );
        file.open( QFile::ReadOnly );
        QTextStream str(&file);
        while ( !str.atEnd() )
        {
            bool bOk;
            QString s = str.readLine();
            if ( s.trimmed().isEmpty() )
                continue;
            bOk = false;
            int nIndex = s.mid(0,5).toInt( &bOk );
            Q_ASSERT( bOk );
            bOk = false;
            int nPairs = s.mid(5,3).toInt( &bOk );
            Q_ASSERT( bOk );
            
            // Check we have the whole string
            int nOffset = 8;
            while ( s.length() < nOffset + nPairs * 2 && !str.atEnd() )
                s += str.readLine();
            
            HersheyChar &c = m_mapCharacterSet.insert( nIndex, HersheyChar() ).value();
            for ( int i = 0; i < nPairs && nOffset < s.length(); i++)
            {
                char c1 = s.at(nOffset++).toAscii();
                char c2 = s.at(nOffset++).toAscii();
                
                if ( i == 0 )
                {
                    int left = 'R' - c1;
                    int right = 'R' - c2;
                    
                    c.m_left = left;
                    c.m_right = right;
                    
                    c.m_Lines.append( QVector<HersheyPoint>() );
                }
                else
                {
                    if ( c1 == ' ' && c2 == 'R' )
                    {
                        // UP
                        c.m_Lines.append( QVector<HersheyPoint>() );
                    }
                    else
                    {
                        int x = 'R' - c1;
                        int y = 'R' - c2;
                        
                        c.m_Lines.last().append( HersheyPoint(x,y) );
                    }
                }
            }
        }
    }
};

HersheyFontBase *HersheyFont::m_pFontBase = NULL;

HersheyFont::HersheyFont()
{
    if ( m_pFontBase == NULL )
        m_pFontBase = new HersheyFontBase();
        
    m_pCharMap = new HersheyCharacterMap();
    m_pCharMap->LoadMap(g_pAppSettings->CharMapPath());
    CalcFontScale();
}

void HersheyFont::CalcFontScale()
{
    // Use 'X' to compute the scale and offset of the font to set Height to 0 - 1.0
    int ymin = 1000;
    int ymax = -1000;
    QString s = "X";
    foreach ( QChar c, s )
    {
        int n = c.toAscii();
        const HersheyChar &hc = m_pFontBase->m_mapCharacterSet[ m_pCharMap->m_mapMap[n] ];
        
        foreach ( const QVector<HersheyPoint> &line, hc.m_Lines )
        {
            foreach ( const HersheyPoint &pt, line )
            {
                if ( pt.y > ymax )
                    ymax = pt.y;
                else if ( pt.y < ymin )
                    ymin = pt.y;
            }
        }
    }
    m_yoffset = -ymin;
    m_yscale = 1.0 / (double)(ymax - ymin);
}

void HersheyFont::plot_string(const QString &s, double /*frac*/) const
{
    //double mat[4*4];
    // TODO - this should be in GLHersheyFont derived class
    glPushMatrix();
//    glGetDoublev(GL_MODELVIEW_MATRIX, mat);
//    if ( mat[10] > .001 )
//    {
//        glTranslatef(0, .5, 0);
//        glRotatef(180, 0, 1, 0);
//        glTranslatef(0, -.5, 0);
//        //frac = 1 - frac;
//        glGetDoublev(GL_MODELVIEW_MATRIX, mat );
//    }
//    if ( mat[5] > .001 )
//    {
//        glTranslatef(0, .5, 0);
//        glRotatef(180, 0, 0, 1);
//        glTranslatef(0, -.5, 0);
//        // frac = 1 - frac
//    }
        
        
    foreach ( QChar c, s )
    {
        int n = c.toAscii();
        const HersheyChar &hc = m_pFontBase->m_mapCharacterSet[ m_pCharMap->m_mapMap[n] ];
        
        double w = (hc.m_right - hc.m_left) * m_yscale;
        //qDebug() << "Char " << c << "(" << n << ") " << hc.m_left << hc.m_right << w;
        
        foreach ( const QVector<HersheyPoint> &line, hc.m_Lines )
        {
//            qDebug() << "Line";
            glBegin(GL_LINE_STRIP);
            foreach ( const HersheyPoint &pt, line )
            {
                double x = (pt.x - hc.m_left) * m_yscale;
                double y = (pt.y + m_yoffset) * m_yscale;
                //qDebug() << x << "," << y;
                //qDebug() << pt.x << "," << pt.y;
                glVertex3d(x,y,0);
            }
            glEnd();
        }
        glTranslated(w, 0, 0);
    }
    glPopMatrix();
}

