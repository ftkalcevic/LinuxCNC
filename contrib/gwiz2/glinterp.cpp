#include "stdafx.h"
#include "glinterp.h"

//#include <GL/glut.h>
#include <math.h>


GLInterp::GLInterp()
    : InterpBaseClass( CANON_UNITS_MM )
    , m_units( CANON_UNITS_MM )
{
    m_extentsMin.pts[0] = m_extentsMin.pts[1] = m_extentsMin.pts[2] = 0;
    m_extentsMax.pts[0] = m_extentsMax.pts[1] = m_extentsMax.pts[2] = 1.5;
    ComputeSizeAndMid();
}


void GLInterp::SetLastPoint( double x, double y, double z )
{
    m_vLastPoint.pts[0] = x;
    m_vLastPoint.pts[1] = y;
    m_vLastPoint.pts[2] = z;
}

void GLInterp::AppendLine( MoveType::MoveType type, double x, double y, double z )
{
    GLVertex3D v(x,y,z);
 
    m_extentsMin.pts[0] = fmin( x, m_extentsMin[0] );
    m_extentsMin.pts[1] = fmin( y, m_extentsMin[1] );
    m_extentsMin.pts[2] = fmin( z, m_extentsMin[2] );
    m_extentsMax.pts[0] = fmax( x, m_extentsMax[0] );
    m_extentsMax.pts[1] = fmax( y, m_extentsMax[1] );
    m_extentsMax.pts[2] = fmax( z, m_extentsMax[2] );
    
    if ( type != m_eLastMoveType )
    {
        // New vector
        m_WizLineGroup.last().m_strips[type].append( GLLineStrip() );
        m_WizLineGroup.last().m_strips[type].last().strip.append( m_vLastPoint );
        m_eLastMoveType = type;
    }
    m_WizLineGroup.last().m_strips[type].last().strip.append( v );
    m_vLastPoint = v;
}

void GLInterp::InitCanon()
{
    m_nWizLineNo = 0;
    m_bHasLastPoint = false;
    m_vLastPoint.pts[0] = m_vLastPoint.pts[1] = m_vLastPoint.pts[2] = 0.0;
    m_eLastMoveType = MoveType::MoveType_Max;
    m_WizLineGroup.clear();
    m_WizLineGroup.append( WizLineLines() );
}

void GLInterp::ArcFeed( int line_number,
                        double first_end, double second_end, double first_axis,
                        double second_axis, int rotation, double axis_end_point,
                        double a_position, double b_position, double c_position,
                        double u_position, double v_position, double w_position )
{
    // Subdivide the arc into line segments.
    qDebug() << line_number << "ArcFeed";
    rs274_arc_to_segments( line_number, first_end, second_end, 
                           first_axis, second_axis,
                           rotation, axis_end_point,
                           a_position, b_position, c_position,
                           u_position, v_position, w_position );
}

void GLInterp::StraightFeed( int line_number,
                             double x, double y, double z,
                             double /*a*/, double /*b*/, double /*c*/,
                             double /*u*/, double /*v*/, double /*w*/ )
{
    AppendLine( MoveType::Feed, x, y, z );
    qDebug() << line_number << "StraightFeed" << x << y << z;
}

void GLInterp::StraightTraverse( int line_number,
                                 double x, double y, double z,
                                 double /*a*/, double /*b*/, double /*c*/,
                                 double /*u*/, double /*v*/, double /*w*/ ) 
{
    AppendLine( MoveType::Traverse, x, y, z );
    qDebug() << line_number << "StraightTraverse" << x << y << z;
}   

void GLInterp::Dwell(double /*time*/) 
{
    AppendLine( MoveType::Dwell, CurrentPos().tran.x, CurrentPos().tran.y, CurrentPos().tran.z );
}

void GLInterp::Comment(const char *c)
{
    // Look for the comment tag that shows the start of a macro output
    const char *sWizLineNo = strstr( c, GWIZARD_TRACKING_TOKEN );
    if ( sWizLineNo )
    {
        //int nWizLineNo = atoi( sWizLineNo );
        qDebug() << "WizLine " << sWizLineNo;
        m_WizLineGroup.append( WizLineLines() );
        m_eLastMoveType = MoveType::MoveType_Max;
    }
}

void GLInterp::NurbsFeed(int /*line_number*/, std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k)
{
    double u = 0.0;
    unsigned int n = nurbs_control_points.size() - 1;
    double umax = n - k + 2;
    unsigned int div = nurbs_control_points.size()*15;
    std::vector<unsigned int> knot_vector = knot_vector_creator(n, k);	
    PLANE_POINT P1;
    double current_z = CurrentPos().tran.z;
    while (u+umax/div < umax) 
    {
        PLANE_POINT P1 = nurbs_point(u+umax/div,k,nurbs_control_points,knot_vector);
        AppendLine( MoveType::Nurbs, P1.X, P1.Y, current_z );
        u = u + umax/div;
    } 
    P1.X = nurbs_control_points[n].X;
    P1.Y = nurbs_control_points[n].Y;
    AppendLine( MoveType::Nurbs, P1.X, P1.Y, current_z );
}

void GLInterp::CanonError(const char *fmt, va_list arg ) 
{
    char buf[2048];
    vsnprintf( buf, sizeof(buf), fmt, arg );
    qDebug() << "ERROR: " << buf;
    QMessageBox::warning( NULL, "GCode Error", QString(buf) );
} 

void GLInterp::NextLine(int /*line_number*/)
{
    //qDebug() << "N" << line_number;
}

void GLInterp::UseLengthUnits(CANON_UNITS u)
{
    qDebug() << "UseLengthUnits " << u;
    m_units = u;
}

CANON_TOOL_TABLE GLInterp::GetExternalToolTable(int pocket)
{
    // Fake tool table
    CANON_TOOL_TABLE t = {pocket,{{0,0,0},0,0,0,0,0,0},0,0,0,0}; 
    return t;
}

void GLInterp::unrotate(double &x, double &y, double c, double s) 
{
    double tx = x * c + y * s;
    y = -x * s + y * c;
    x = tx;
}

void GLInterp::rotate(double &x, double &y, double c, double s) 
{
    double tx = x * c - y * s;
    y = x * s + y * c;
    x = tx;
}

void GLInterp::EmcPose2Array( const EmcPose &pos, double o[9] )
{
    o[0] = pos.tran.x;
    o[1] = pos.tran.y;
    o[2] = pos.tran.z;
    o[3] = pos.a;
    o[4] = pos.b;
    o[5] = pos.c;
    o[6] = pos.u;
    o[7] = pos.v;
    o[8] = pos.w;
}

void GLInterp::rs274_arc_to_segments( int line_number, double x1, double y1, double cx, double cy, int number_rotations, double z1, double a, double b, double c, double u, double v, double w ) 
{
    double o[9], n[9], g5xoffset[9], g92offset[9];
    int X, Y, Z;
    int max_segments = 128;
    
    int plane = InterpBaseClass::Plane(); 
    double rotation_cos = cos(InterpBaseClass::XYRotation());  
    double rotation_sin = sin(InterpBaseClass::XYRotation()); 
    EmcPose2Array( InterpBaseClass::CurrentPos(), o );
    EmcPose2Array( InterpBaseClass::g5xOffset(), g5xoffset );
    EmcPose2Array( InterpBaseClass::g92Offset(), g92offset );


    if ( plane == CANON_PLANE_XY ) 
    {
        X=0; Y=1; Z=2;
    } 
    else if ( plane == CANON_PLANE_XZ )
    {
        X=2; Y=0; Z=1;
    } 
    else // CANON_PLANE_YZ, etal
    {
        X=1; Y=2; Z=0;
    }
    
    n[X] = x1;
    n[Y] = y1;
    n[Z] = z1;
    n[3] = a;
    n[4] = b;
    n[5] = c;
    n[6] = u;
    n[7] = v;
    n[8] = w;
    for(int ax=0; ax<9; ax++) 
        o[ax] -= g5xoffset[ax];
    unrotate(o[0], o[1], rotation_cos, rotation_sin);
    for(int ax=0; ax<9; ax++) 
        o[ax] -= g92offset[ax];

    double theta1 = atan2(o[Y]-cy, o[X]-cx);
    double theta2 = atan2(n[Y]-cy, n[X]-cx);

    if(number_rotations < 0) 
    {
        while(theta2 - theta1 > -CIRCLE_FUZZ) 
        {
            theta2 -= 2*M_PI;
        }
    } 
    else 
    {
        while(theta2 - theta1 < CIRCLE_FUZZ) 
        {
            theta2 += 2*M_PI;
        }
    }

    // if multi-turn, add the right number of full circles
    if(number_rotations < -1) 
    {
        theta2 += 2*M_PI*(number_rotations+1);
    }
    if(number_rotations > 1) 
    {
        theta2 += 2*M_PI*(number_rotations-1);
    }

    int steps = std::max(3, int(max_segments * fabs(theta1 - theta2) / M_PI));
    double rsteps = 1. / steps;

    double dtheta = theta2 - theta1;
    double d[9] = {0, 0, 0, n[3]-o[3], n[4]-o[4], n[5]-o[5], n[6]-o[6], n[7]-o[7], n[8]-o[8]};
    d[Z] = n[Z] - o[Z];

    double tx = o[X] - cx, ty = o[Y] - cy, dc = cos(dtheta*rsteps), ds = sin(dtheta*rsteps);
    for(int i=0; i<steps-1; i++) 
    {
        double f = (i+1) * rsteps;
        double p[9];
        rotate(tx, ty, dc, ds);
        p[X] = tx + cx;
        p[Y] = ty + cy;
        p[Z] = o[Z] + d[Z] * f;
        p[3] = o[3] + d[3] * f;
        p[4] = o[4] + d[4] * f;
        p[5] = o[5] + d[5] * f;
        p[6] = o[6] + d[6] * f;
        p[7] = o[7] + d[7] * f;
        p[8] = o[8] + d[8] * f;
        for(int ax=0; ax<9; ax++) 
        {
            p[ax] += g92offset[ax];
        }
        rotate(p[0], p[1], rotation_cos, rotation_sin);
        for(int ax=0; ax<9; ax++) p[ax] += g5xoffset[ax];
        
        StraightFeed( line_number, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8] );
    }
    for(int ax=0; ax<9; ax++) 
    {
        n[ax] += g92offset[ax];
    }
    rotate(n[0], n[1], rotation_cos, rotation_sin);
    for(int ax=0; ax<9; ax++) 
    {
        n[ax] += g5xoffset[ax];
    }
    StraightFeed( line_number, n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8] );
}


void GLInterp::ComputeSizeAndMid()
{
    m_extentsSize.pts[0] = m_extentsMax[0] - m_extentsMin[0];
    m_extentsSize.pts[1] = m_extentsMax[1] - m_extentsMin[1];
    m_extentsSize.pts[2] = m_extentsMax[2] - m_extentsMin[2];
    m_extentsMid.pts[0] = (m_extentsMax[0] + m_extentsMin[0])/2;
    m_extentsMid.pts[1] = (m_extentsMax[1] + m_extentsMin[1])/2;
    m_extentsMid.pts[2] = (m_extentsMax[2] + m_extentsMin[2])/2;
}

void GLInterp::parse_file( const char *filename )
{
    // Default to the Axis display
    m_extentsMin.pts[0] = m_extentsMin.pts[1] = m_extentsMin.pts[2] = -0.5;
    m_extentsMax.pts[0] = m_extentsMax.pts[1] = m_extentsMax.pts[2] = 1.5;
    
    InterpBaseClass::parse_file( filename );
    
    ComputeSizeAndMid();
}

DrawColour::DrawColours GLInterp::ColourIndex( MoveType::MoveType eType, bool bHighlight ) 
{
    if ( bHighlight )
    {
        switch ( eType )
        {
            case MoveType::Feed:      return DrawColour::FeedHighlightColour;
            case MoveType::Arc:       return DrawColour::ArcHighlightColour;
            case MoveType::Traverse:  return DrawColour::TraverseHighlightColour;
            case MoveType::Nurbs:     return DrawColour::NurbsHighlightColour;
            case MoveType::Dwell:     return DrawColour::DwellHighlightColour;
            case MoveType::MoveType_Max:
            default:
                Q_ASSERT(false);
        }
    }
    else
    {
        switch ( eType )
        {
            case MoveType::Feed:      return DrawColour::FeedColour;
            case MoveType::Arc:       return DrawColour::ArcColour;
            case MoveType::Traverse:  return DrawColour::TraverseColour; 
            case MoveType::Nurbs:     return DrawColour::NurbsColour;
            case MoveType::Dwell:     return DrawColour::DwellColour;
            case MoveType::MoveType_Max:
            default:
                Q_ASSERT(false);
        }
    }
    return DrawColour::FeedColour;
}

void GLInterp::DrawRowLines( bool bHighlight, const WizLineLines &wl )
{
    // TODO why does this crash the debugger
#ifndef DEBUG
    if ( bHighlight )
        glLineWidth( 3.0 );
    else
#endif
        glLineWidth( 1.0 );
    
    for ( int i = 0; i < MoveType::MoveType_Max; i++ )
    {
        if ( !wl.m_strips[i].isEmpty() )
        {
            glColor4dv( g_pAppSettings->Colour( ColourIndex( (MoveType::MoveType)i, bHighlight ) ) );

            // special for dwells
            if ( i == MoveType::Dwell )
            {
                double delta = 0.015625;   // 1/2 length of dwell symbol
                glBegin(GL_LINES);
                foreach ( const GLLineStrip &ls, wl.m_strips[i])
                {
                    foreach( const GLVertex3D &v, ls.strip )
                    {
                        // todo - orient according to view
                        glVertex3d( v[0]+delta, v[1], v[2]+delta );
                        glVertex3d( v[0]-delta, v[1], v[2]-delta );

                        glVertex3d( v[0]-delta, v[1], v[2]+delta );
                        glVertex3d( v[0]+delta, v[1], v[2]-delta );
                    }
                }
                glEnd();
            }
            else
            {
                foreach ( const GLLineStrip &ls, wl.m_strips[i])
                {
                    glBegin(GL_LINE_STRIP);
                    foreach( const GLVertex3D &v, ls.strip )
                    {
                        glVertex3dv( v.pts );
                    }
                    glEnd();
                }
            }
        }
    }
}

void GLInterp::DrawLines( int nHighlightRow )
{
    int nRow = 0;
    foreach ( const WizLineLines &wl, m_WizLineGroup )
    {
        DrawRowLines( nRow == nHighlightRow, wl );
        nRow++;
    }
}
