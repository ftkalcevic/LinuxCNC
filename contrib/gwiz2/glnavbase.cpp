#include "stdafx.h"
#include "glnavbase.h"
#include "GL/gl.h"
#include "GL/glu.h"
#include <math.h>

// From glnav.py

static double rotation_vectors[2][3] = { {1.,0.,0.}, {0., 0., 1.}};


GLNavBase::GLNavBase()
{
    // Current coordinates of the mouse.
    xmouse = 0;
    ymouse = 0;

    // Where we are centering.
    xcenter = 0.0;
    ycenter = 0.0;
    zcenter = 0.0;

    // The _back color
    r_back = 1.0;
    g_back = 0.0;
    b_back = 1.0;

    // Where the eye is
    distance = 10.0;

    // Field of view in y direction
    fovy = 30.0;

    // Position of clipping planes.
    near = 0.1;
    far = 1000.0;

    // View settings
    perspective = true;
    lat = 0.0;
    lon = 0.0;
    minlat = -90.0;
    maxlat = 90.0;
}


  
//# This should almost certainly be part of some derived class.
//# But I have put it here for convenience.
//def basic_lighting(self):
//    """
//    Set up some basic lighting (single infinite light source).

//    Also switch on the depth buffer."""

//    self.activate()
//    glLightfv(GL_LIGHT0, GL_POSITION, (1, -1, 1, 0))
//    glLightfv(GL_LIGHT0, GL_AMBIENT, (.4, .4, .4, 1))
//    glLightfv(GL_LIGHT0, GL_DIFFUSE, (.6, .6, .6, 1))
//    glEnable(GL_LIGHTING)
//    glEnable(GL_LIGHT0)
//    glDepthFunc(GL_LESS)
//    glEnable(GL_DEPTH_TEST)
//    glMatrixMode(GL_MODELVIEW)
//    glLoadIdentity()


void GLNavBase::set_background( float r, float g, float b )
{
    // Change the background colour of the widget.
    r_back = r;
    g_back = g;
    b_back = b;
    invalidateScene();
}


void GLNavBase::set_centerpoint( double x, double y, double z)
{
    // Set the new center point for the model.
    // This is where we are looking.

    xcenter = x;
    ycenter = y;
    zcenter = z;

    invalidateScene();
}

void GLNavBase::set_latitudelimits(double new_minlat, double new_maxlat)
{
    // Set the new "latitude" limits for rotations.

    if (new_maxlat > 180 )
        return;
    if ( new_minlat < -180 )
        return;
    if ( new_maxlat <= new_minlat )
        return;
    maxlat = new_maxlat;
    minlat = new_minlat;

    invalidateScene();
}

void GLNavBase::set_eyepoint( double new_distance)
{
    // Set how far the eye is from the position we are looking.

    distance = new_distance;
    invalidateScene();
}


void GLNavBase::set_eyepoint_from_extents( double e1, double e2 )
{
    // Set how far the eye is from the position we are looking
    // based on the screen width and height of a subject.
    int w = wininfo_width();
    int h = wininfo_height();
    if ( w == 0 ) w = 1;
    if ( h == 0 ) h = 1;

    double wh_ratio = (double)w / (double)h;
    double e12_ratio = e1/e2;

    double a;
    if ( wh_ratio > e12_ratio )
        a = e2;
    else
        a = e1;
        
    double w2 = a/2.0;
    double d = w2/tan(fovy/180.0*M_PI/2.0);
    d *= 1.1;       // 10% border
    
    set_eyepoint(d-zcenter);
}

void GLNavBase::reset()
{
    // Reset rotation matrix for this widget.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    invalidateScene();
}

void GLNavBase::recordMouse( int x, int y )
{
    xmouse = x;
    ymouse = y;
}

void GLNavBase::startRotate( int x, int y )
{
    recordMouse(x, y);
}

void GLNavBase::scale( int x, int y )
{
    // Scale the scene.  Achieved by moving the eye position.
    // Dragging up zooms in, while dragging down zooms out
    double scale = 1.0 - 0.01 * (y - ymouse);
            
    // do some sanity checks, scale no more than
    // 1:1000 on any given click+drag
    if ( scale < 0.001 )
        scale = 0.001;
    else if ( scale > 1000 )
        scale = 1000;
    
    double newdistance = distance * scale;
    if ( newdistance < 1e-30 || newdistance > 1e30 )
        return;
    distance = newdistance;
    invalidateScene();
    recordMouse(x, y);
}

void GLNavBase::rotate( int x, int y)
{
    // Perform rotation of scene.
    //self.activate()
    perspective = true;
    glRotateScene( 0.5, xcenter, ycenter, zcenter, x, y, xmouse, ymouse);
    invalidateScene();
    recordMouse(x, y);
}

void GLNavBase::translate( int x, int y )
{
    // Perform translation of scene.

    //self.activate()
    int h = wininfo_height();

    // Scale mouse translations to object viewplane so object tracks with mouse
    double win_height = fmax( 1.0, h );
    GLVertex3D obj_c( xcenter, ycenter, zcenter );
    GLVertex3D win = GLUtility::gluProject( obj_c );
    GLVertex3D obj = GLUtility::gluUnProject( win[0], win[1] + 0.5 * win_height, win[2]);
    double dist = sqrt( GLUtility::v3distsq( obj, obj_c ) );
    double scale = fabs( dist / ( 0.5 * (double)h ) );

    glTranslateScene( scale, x, y, xmouse, ymouse );
    invalidateScene();
    recordMouse(x, y);
}


void GLNavBase::set_viewangle(double new_lat, double /*new_lon*/)
{
    lat = new_lat;
    glRotateScene( 0.5, xcenter, ycenter, zcenter, 0, 0, 0, 0);
    invalidateScene();
}

void GLNavBase::zoomin()
{
    distance = distance / 1.1;
    invalidateScene();
}

void GLNavBase::zoomout()
{
    distance = distance * 1.1;
    invalidateScene();
}

void GLNavBase::startZoom(int y)
{
    y0 = y;
    original_zoom = distance;
}

void GLNavBase::continueZoom( int y )
{
    int dy = y - y0;
    distance = original_zoom * pow(1.25, (double)dy / 16.0 );
    invalidateScene();
}

bool GLNavBase::getRotateMode()
{
    return false;
}

void GLNavBase::translateOrRotate( int x, int y)
{
    if ( getRotateMode() )
        rotate(x, y);
    else
        translate(x, y);
}
      
void GLNavBase::rotateOrTranslate( int x, int y)
{
    if ( ! getRotateMode() )
        rotate(x, y);
    else
        translate(x, y);
}
      
void GLNavBase::setView( ViewType::ViewTypes v )
{
    switch ( v )
    {
        case ViewType::X:   set_view_x(); break;
        case ViewType::Y:   set_view_y(); break;
        case ViewType::Z:   set_view_z(); break;
        case ViewType::Z2:  set_view_z2(); break;
        case ViewType::P:   set_view_p(); break;
    }
    view = v;
}

void GLNavBase::set_view_x()    // YZ plane
{
    reset();
    glRotatef(-90, 0, 1, 0);
    glRotatef(-90, 1, 0, 0);
    GLVertex3D mid = extentsinfo_mid();
    GLVertex3D size = extentsinfo_size();
    glTranslatef(-mid[0], -mid[1], -mid[2]);
    set_eyepoint_from_extents(size[1], size[2]);
    perspective = false;
    lat = -90;
    lon = 270;
    invalidateScene();
}


void GLNavBase::set_view_y()    // ZX plane
{
    reset();
    glRotatef(-90, 1, 0, 0);
    if ( is_lathe() )
        glRotatef(90, 0, 1, 0);
    GLVertex3D mid = extentsinfo_mid();
    GLVertex3D size = extentsinfo_size();
    glTranslatef(-mid[0], -mid[1], -mid[2]);
    if ( is_lathe() )
        set_eyepoint_from_extents(size[2], size[0]);
    else
        set_eyepoint_from_extents(size[0], size[2]);
    perspective = false;
    lat = -90;
    lon = 0;
    invalidateScene();
}

void GLNavBase::set_view_z()    // XY plane
{
    reset();
    GLVertex3D mid = extentsinfo_mid();
    GLVertex3D size = extentsinfo_size();
    glTranslatef(-mid[0], -mid[1], -mid[2]);
    set_eyepoint_from_extents(size[0], size[1]);
    perspective = false;
    lat = lon = 0;
    invalidateScene();
}

void GLNavBase::set_view_z2()
{
    reset();
    glRotatef(-90, 0, 0, 1);
    GLVertex3D mid = extentsinfo_mid();
    GLVertex3D size = extentsinfo_size();
    glTranslatef(-mid[0], -mid[1], -mid[2]);
    set_eyepoint_from_extents(size[1], size[0]);
    perspective = false;
    lat = 0;
    lon = 270;
    invalidateScene();
}

void GLNavBase::set_view_p()
{
    reset();
    perspective = true;
    GLVertex3D vMid = extentsinfo_mid();
    GLVertex3D vSize = extentsinfo_size();
    glTranslatef(-vMid[0], -vMid[1], -vMid[2]);
    double size = sqrt( fsqr(vSize[0]) + fsqr(vSize[1]) + fsqr(vSize[2]) );
    if ( size > 1e99) 
        size = 5.0;    //in case there are no moves in the preview
    int w = wininfo_width();
    int h = wininfo_height();
    double fovx = fovy * w / h;
    double fov = fmin(fovx, fovy);
    set_eyepoint( (size * 1.1 + 1.0) / 2.0 / sin( fov * M_PI / 180.0 / 2.0));
    lat = -60;
    lon = 335;
    glRotateScene( 1.0, vMid[0], vMid[1], vMid[2], 0, 0, 0, 0);
    invalidateScene();
}


void GLNavBase::glTranslateScene( double scale, int x, int y, int mousex, int mousey)
{
    glMatrixMode(GL_MODELVIEW);
    double mat[16];
    glGetDoublev(GL_MODELVIEW_MATRIX,mat);
    glLoadIdentity();
    glTranslatef(scale * (x - mousex), scale * (mousey - y), 0.0);
    glMultMatrixd(mat);
}

static double snap( double a )
{
    double m = fmod( a, 90.0 );
    qDebug() << a << "," << m;
    if ( fabs(m) < 3 )
        return a-m;
    else if ( fabs(m) > 87 )
        return a-m+90;
    else
        return a;
}

void GLNavBase::glRotateScene( double /*scale*/, double xcenter, double ycenter, double zcenter, int x, int y, int mousex, int mousey)
{
    double new_lat = fmin( maxlat, fmax( minlat, lat + (double)(y - mousey) * .5));
    double new_lon = fmod( (lon + (double)(x - mousex) * .5), 360 );

    glMatrixMode(GL_MODELVIEW);

    glTranslatef(xcenter, ycenter, zcenter);
    double mat[4*4];
    glGetDoublev(GL_MODELVIEW_MATRIX, mat);

    glLoadIdentity();
    GLdouble tx = mat[12];
    GLdouble ty = mat[13];
    GLdouble tz = mat[14];
    glTranslatef(tx, ty, tz);
    GLUtility::glRotatedv( snap(new_lat), rotation_vectors[0] );
    GLUtility::glRotatedv( snap(new_lon), rotation_vectors[1] );
    glTranslatef(-xcenter, -ycenter, -zcenter);
    lat = new_lat;
    lon = new_lon;
}


void GLNavBase::redraw_perspective()
{
    int w = wininfo_width();
    int h = wininfo_height();
    glViewport(0, 0, w, h);

    // Clear the background and depth buffer.
    GLRGBA c = g_pAppSettings->Colour(DrawColour::BackColour);
    glClearColor( c[0], c[1], c[2], c[3] );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, float(w)/float(h), near, far + distance);

    gluLookAt(0, 0, distance,
        0, 0, 0,
        0., 1., 0.);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    drawScene();

    glFlush();      // Tidy up
    glPopMatrix();  // Restore the matrix
}
      
      
void GLNavBase::redraw_ortho()
{
    int w = wininfo_width();
    int h = wininfo_height();
    glViewport(0, 0, w, h);

    // Clear the background and depth buffer.
    GLRGBA c = g_pAppSettings->Colour(DrawColour::BackColour);
    glClearColor( c[0], c[1], c[2], c[3] );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double l = tan(fovy/2*M_PI/180) * distance;
    double k = l * (double)w / (double)h;
    glOrtho(-k, k, -l, l, -1000, 1000.);

    gluLookAt(0, 0, 1,
        0, 0, 0,
        0., 1., 0.);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    drawScene();

    glFlush();      // Tidy up
    glPopMatrix();  // Restore the matrix
}


void GLNavBase::redrawScene()
{
    if ( getView() == ViewType::P )
        redraw_perspective();
    else
        redraw_ortho();
}
