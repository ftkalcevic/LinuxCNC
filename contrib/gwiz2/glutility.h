#ifndef GLUTILITY_H
#define GLUTILITY_H

#include "GL/gl.h"
#include "GL/glu.h"

static GLdouble fsqr( const GLdouble a ) { return a*a; }


struct GLVertex3D
{
    // OpenGL vertex
    GLdouble pts[3];
    
    GLVertex3D()
    {
        pts[0] = 0;
        pts[1] = 0;
        pts[2] = 0;
    }

    GLVertex3D(double x, double y, double z )
    {
        pts[0] = x;
        pts[1] = y;
        pts[2] = z;
    }
    
    operator const GLdouble *() { return pts; }
    GLdouble operator [] (int n) const { return pts[n]; }
      
};

inline QDebug operator<<(QDebug dbg, const GLVertex3D &v)
{
    dbg.nospace() << "(" << v.pts[0] << ", " << v.pts[1] << ", " << v.pts[2] << ")";

    return dbg.space();
}

struct GLRGBA
{
    // OpenGL RGBA colour
    GLdouble rgba[4];
    
    GLRGBA()
    {
        rgba[0] = 0;
        rgba[1] = 0;
        rgba[2] = 0;
        rgba[3] = 0;
    }

    GLRGBA(const QString &s)
    {
        QStringList digits = s.split( ",");
        if ( digits.count() >= 4 )
        {
            rgba[0] = digits[0].toDouble();
            rgba[1] = digits[1].toDouble();
            rgba[2] = digits[2].toDouble();
            rgba[3] = digits[3].toDouble();
        }
    }

    GLRGBA(double r, double g, double b, double a )
    {
        rgba[0] = r;
        rgba[1] = g;
        rgba[2] = b;
        rgba[3] = a;
    }
    
    operator const GLdouble *() { return rgba; }
    GLdouble operator [] (int n) const { return rgba[n]; }
};

class GLUtility
{
public:
    GLUtility();
    static GLVertex3D gluProject(GLVertex3D v);
    static GLVertex3D gluUnProject(GLVertex3D v);
    static GLVertex3D gluUnProject(GLdouble x,GLdouble y,GLdouble z) { return gluUnProject(GLVertex3D(x,y,z)); }
    
    static GLdouble v3distsq( const GLVertex3D &a, const GLVertex3D &b) 
    {
        return fsqr(a[0] - b[0]) + fsqr(a[1] - b[1]) + fsqr(a[2] - b[2] );
    }
    static void glRotatedv( double angle, GLdouble *axis ) { glRotated( angle, axis[0], axis[1], axis[2] ); }
    
};

#endif // GLUTILITY_H
