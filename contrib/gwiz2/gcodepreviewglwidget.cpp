#include "stdafx.h"
#include "gcodepreviewglwidget.h"

#include <QMouseEvent>
#include <math.h>

GCodePreviewGLWidget::GCodePreviewGLWidget(bool bLathe, QWidget *parent) 
    : QGLWidget(parent)
    , m_bLathe( bLathe )
    , m_bGrid(false)
    , m_highlightedLine(-1)
    , m_pInterp( new GLInterp() )
{
    SetDefaultView();
}

GCodePreviewGLWidget::~GCodePreviewGLWidget()
{
}

GLVertex3D GCodePreviewGLWidget::extentsinfo_mid()  
{ 
    if ( m_pInterp )
        return m_pInterp->GetExtentsMid();      
    else
        return GLVertex3D(1,1,1);   // todo get defaults from m_pinterp - create earlier
}
GLVertex3D GCodePreviewGLWidget::extentsinfo_size() 
{ 
    if ( m_pInterp )
        return m_pInterp->GetExtentsSize();
    else
        return GLVertex3D(3,3,3); 
}

void GCodePreviewGLWidget::SetDefaultView()
{
    if ( is_lathe() )
        setView( ViewType::Y );
    else    
        setView( ViewType::P );
}

void GCodePreviewGLWidget::Init()
{
    SetDefaultView();
}

void GCodePreviewGLWidget::Load( const char * sFilename, int hightlight_line, bool bResetView )
{
    if ( m_pInterp == NULL )
        m_pInterp = new GLInterp();
    m_pInterp->parse_file( sFilename );
    
    if ( bResetView )
        SetDefaultView();    
    
    m_highlightedLine = hightlight_line;
    
    updateGL();
}



void GCodePreviewGLWidget::initializeGL()
{
    
//    qglClearColor(qtPurple.dark());
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    logo = new QtLogo(this, 64);
//    logo->setColor(qtGreen.dark());
/*
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_MULTISAMPLE);
    static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    */
    float pos[] = {1, -1, 1};
    float ambient[] = {.5,.5,.5,0.5};
    float diffuse[] = {.5,.5,.5,0.5};
    float material[] = {1,1,1,0};
    
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material);
    //glEnable(GL_CULL_FACE);
    //glEnable(GL_MULTISAMPLE);
    //glShadeModel(GL_FLAT);
    glShadeModel(GL_FLAT);
    
    //glEnable(GL_LIGHTING);
    //glEnable(GL_LIGHT0);

    glDepthFunc(GL_NEVER);
    //glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GCodePreviewGLWidget::ComputeGridParams( double ext_min, double ext_max, int &min, int &max, int &large_step, int &small_step )
{
    // try lots of 10.
    min = floor((ext_min+.1)/10.0) * 10.0;
    max = ceil((ext_max+.1)/10.0) * 10.0;
    large_step = 1;
    small_step = 10;
}

void GCodePreviewGLWidget::DrawGrid( double ext_min_x, double ext_max_x, double ext_min_y, double ext_max_y )
{
    // Get the extents
    int min_x, max_x, large_step_x, small_step_x;
    ComputeGridParams( ext_min_x, ext_max_x, min_x, max_x, large_step_x, small_step_x );
    int min_y, max_y, large_step_y, small_step_y;
    ComputeGridParams( ext_min_y, ext_max_y, min_y, max_y, large_step_y, small_step_y );
    
    GLVertex3D ss1 = GLUtility::gluProject( GLVertex3D(0,0,0) );
    GLVertex3D ss2 = GLUtility::gluProject( GLVertex3D(1.0/(double)small_step_x,0,0) );
    bool bShowSmallGrid = sqrt( GLUtility::v3distsq( ss1, ss2 ) ) > g_pAppSettings->MinPhysicalGridSpacing();

    GLVertex3D ls1 = GLUtility::gluProject( GLVertex3D(0,0,0) );
    GLVertex3D ls2 = GLUtility::gluProject( GLVertex3D(1.0/(double)large_step_x,0,0) );
    bool bShowLargeGrid = sqrt( GLUtility::v3distsq( ls1, ls2 ) ) > g_pAppSettings->MinPhysicalGridSpacing();
    
    glLineWidth( 1.0 );
    glBegin(GL_LINES);
    
    // Major Axis
    if ( (min_x <=0 && max_x >= 0) ||
         (min_y <=0 && max_y >= 0) )
    {
        glColor4d( 1.0, 1.0, 1.0, .2 );
        
        if ( min_x <=0 && max_x >= 0 )
        {
            glVertex3d(0,min_y,0);        
            glVertex3d(0,max_y,0);
        }
        if ( min_y <=0 && max_y >= 0 )
        {
            glVertex3d(min_x,0,0);        
            glVertex3d(max_x,0,0);
        }
    }
    
    // Major Grid
    if ( bShowLargeGrid )
    {
        glColor4d( .5, .5, .5, .2 );
        for ( int i = min_x * large_step_x; i <= max_x * large_step_x; i ++ )
        {
            if ( i != 0 )
            {
                glVertex3d((double)i/(double)large_step_x, min_y,0);        
                glVertex3d((double)i/(double)large_step_x, max_y,0);
            }
        }
        for ( int i = min_y * large_step_y; i <= max_y * large_step_y; i ++ )
        {
            if ( i != 0 )
            {
                glVertex3d(min_x,(double)i/(double)large_step_y, 0);        
                glVertex3d(max_x,(double)i/(double)large_step_y, 0);
            }
        }
    }
    glEnd();
    
    // Minor Grid
    if ( bShowSmallGrid )
    {
        glEnable( GL_LINE_STIPPLE );
        glColor4d( .2, .2, .2, .2 );
        glLineStipple( 1, 0xF );
        glBegin(GL_LINES);
        for ( int i = min_x*small_step_x; i <= max_x*small_step_x; i++  )
        {
            if ( (i%small_step_x) != 0 )
            {
                glVertex3d((double)i/(double)small_step_x, min_y,0);        
                glVertex3d((double)i/(double)small_step_x, max_y,0);
            }
        }
        for ( int i = min_y*small_step_y; i <= max_y*small_step_y; i++  )
        {
            if ( (i%small_step_y) != 0 )
            {
                glVertex3d(min_x,(double)i/(double)small_step_y, 0);        
                glVertex3d(max_x,(double)i/(double)small_step_y, 0);
            }
        }
        glEnd();
        glDisable( GL_LINE_STIPPLE );
    }
}

void GCodePreviewGLWidget::DrawGrid()
{
    static struct
    {
        ViewType::ViewTypes v;
        double Rx, Ry, Rz;
        int Ix, Iy;
    } Views[] = {
        { ViewType::X,   0, 90,  90, 1, 2 },   // Viewing YZ plane
        { ViewType::Y, -90,  0, -90, 2, 0 },   // Viewing YZ plane
        { ViewType::Z,   0,  0,   0, 0, 1 },   // Viewing YZ plane
        { ViewType::Z2,  0,  0,   0, 0, 1 },   // Viewing YZ plane
    };
    
    for ( unsigned int i = 0; i < countof(Views); i++ )
        if ( Views[i].v == GetView() )
        {
            glPushMatrix();
            
            if ( Views[i].Rx != 0 )
                glRotatef( Views[i].Rx,  1,0,0 );
            if ( Views[i].Ry != 0 )
                glRotatef( Views[i].Ry,  0,1,0 );
            if ( Views[i].Rz != 0 )
                glRotatef( Views[i].Rz,  0,0,1 );
            
            DrawGrid( m_pInterp->GetExtentsMin()[Views[i].Ix], m_pInterp->GetExtentsMax()[Views[i].Ix], m_pInterp->GetExtentsMin()[Views[i].Iy], m_pInterp->GetExtentsMax()[Views[i].Iy] );
    
            glPopMatrix();
            break;
        }
}
void GCodePreviewGLWidget::DrawAxes()
{
    glLineWidth( 1.0 );
    double dFontScale = 0.2;
    
    // X
    if ( getView() != ViewType::X )
    {
        glColor4dv( g_pAppSettings->Colour( DrawColour::AxisXColour ) );
        glBegin(GL_LINE_STRIP);
            glVertex3d(0,0,0);        
            glVertex3d(1,0,0);
        glEnd();
    
        glPushMatrix();
        if ( is_lathe() )
        {
            glTranslatef(1.3, -0.1, 0);
            glTranslatef(0, 0, -0.1);
            glRotatef(-90, 0, 1, 0);
            glRotatef(90, 1, 0, 0);
            glRotatef(180, 0, 1, 0);
            glTranslatef(0.1, 0, 0);
        }
        else
        {
            glTranslatef(1.2, -0.1, 0);
            if ( getView() == ViewType::Y )
            {
                glTranslatef(0, 0, -0.1);
                glRotatef(90, 1, 0, 0);
            }
        }
        glScalef(dFontScale, dFontScale, dFontScale);
        m_font.plot_string("X",0.5);
        glPopMatrix();
    }
    
    // Y
    glColor4dv( g_pAppSettings->Colour( DrawColour::AxisYColour ) );
    if ( !is_lathe() )
    {
        glBegin(GL_LINE_STRIP);
            glVertex3d(0,0,0);        
            glVertex3d(0,1,0);
        glEnd();
        
        if ( getView() != ViewType::Y )
        {
            glPushMatrix();
            glTranslatef(0, 1.2, 0);
            if ( getView() == ViewType::X )
            {
                glTranslatef(0, 0, -0.1);
                glRotatef(90, 0, 1, 0);
                glRotatef(90, 0, 0, 1);
            }
            glScalef(0.2, 0.2, 0.2);
            m_font.plot_string("Y", 0.5);
            glPopMatrix();
        }
    }
    
    // Z
    glColor4dv( g_pAppSettings->Colour( DrawColour::AxisZColour ) );
    glBegin(GL_LINE_STRIP);
        glVertex3d(0,0,0);        
        glVertex3d(0,0,1);
    glEnd();
    
    if ( getView() != ViewType::Z )
    {
        glPushMatrix();
        glTranslatef(0, 0, 1.2);
        if ( is_lathe() )
        {
            glRotatef(-90, 0, 1, 0);
        }
        if ( getView() == ViewType::X )
        {
            glRotatef(90, 0, 1, 0);
            glRotatef(90, 0, 0, 1);
        }
        else if ( getView() == ViewType::Y || getView() == ViewType::P )
        {
            glRotatef(-90, 1, 0, 0);
        }
        if ( is_lathe() )
        {
            glTranslatef(0, -.1, 0);
        }
        glScalef(0.2, 0.2, 0.2);
        m_font.plot_string("Z", 0.5);
        glPopMatrix();
    }
}

void GCodePreviewGLWidget::drawScene()
{
    if ( m_bGrid && (is_lathe() || getView() != ViewType::P) )
    {
        DrawGrid();
    }
    DrawAxes();
    if ( m_pInterp )
    {
        m_pInterp->DrawLines(m_highlightedLine);
    }    
}

void GCodePreviewGLWidget::paintGL()
{
    static bool bPainting = false;
    Q_ASSERT( !bPainting );
    if ( !bPainting && m_pInterp )
    {
        bPainting = true;

        GLNavBase::redrawScene();
        
        bPainting = false;
    }
}

void GCodePreviewGLWidget::resizeGL(int width, int height)
{
    //int side = qMin(width, height);
    //glViewport((width - side) / 2, (height - side) / 2, side, side);
    glViewport(0,0,width,height);

//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//#ifdef QT_OPENGL_ES_1
//    glOrthof(-0.5, +0.5, -0.5, +0.5, 4.0, 15.0);
//#else
//    glOrtho(-0.5, +0.5, -0.5, +0.5, 4.0, 15.0);
//#endif
//    glMatrixMode(GL_MODELVIEW);
//    gluOrtho2D(0,width,0,height);
    
    // update view
    //SetDefaultView();
}


void GCodePreviewGLWidget::mousePressEvent(QMouseEvent *event)
{
    bool button1 = event->buttons() & Qt::LeftButton;
    bool button2 = event->buttons() & Qt::MiddleButton;
    bool button3 = event->buttons() & Qt::RightButton;

    if ( button1 )
        recordMouse(event->x(), event->y() );
    else if ( button2 )
        recordMouse(event->x(), event->y() );
    else if ( button3 )
        startZoom(event->y());
}

void GCodePreviewGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    bool button1 = event->buttons() & Qt::LeftButton;
    bool button2 = event->buttons() & Qt::MiddleButton;
    bool button3 = event->buttons() & Qt::RightButton;
    bool shift = event->modifiers() & Qt::ShiftModifier;
    bool canRotate = !is_lathe() && perspective;
    
    if ( button1 )
    {
        if ( !shift )
            translate( event->x(), event->y() );
        else if ( canRotate )
            rotate( event->x(), event->y() );
    }
    else if ( button2 && canRotate )
        rotate( event->x(), event->y() );
    else if ( button3 )
        continueZoom(event->y());
}

void GCodePreviewGLWidget::mouseReleaseEvent(QMouseEvent */*event*/)
{
}


void GCodePreviewGLWidget::wheelEvent(QWheelEvent *event)
{
    if ( event->delta() > 0 )   
        zoomin();
    else
        zoomout();
}


void GCodePreviewGLWidget::HighlightLine( int line )
{
    m_highlightedLine = line;
    updateGL();
}


void GCodePreviewGLWidget::SetGrid( bool bGridOn )
{
    m_bGrid = bGridOn;
    updateGL();
}


