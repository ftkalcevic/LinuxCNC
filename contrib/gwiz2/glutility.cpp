#include "stdafx.h"
#include "glutility.h"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>

GLUtility::GLUtility()
{
}

GLVertex3D GLUtility::gluProject( GLVertex3D v )
{
    double wx, wy, wz, model[16], proj[16];
    int viewport[4];
    
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    
    ::gluProject(v[0], v[1], v[2], model, proj, viewport, &wx, &wy, &wz);
    
    return GLVertex3D( wx, wy, wz);
}


GLVertex3D GLUtility::gluUnProject( GLVertex3D v )
{
    double wx, wy, wz, model[16], proj[16];
    int viewport[4];
    
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    
    ::gluUnProject(v[0], v[1], v[2], model, proj, viewport, &wx, &wy, &wz);
    
    return GLVertex3D( wx, wy, wz);
}
