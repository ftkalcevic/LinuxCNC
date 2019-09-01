#ifndef GLINTERP_H
#define GLINTERP_H

#include "interpbaseclass.hh"


namespace MoveType
{
    enum MoveType
    {
        Feed,
        Arc,
        Traverse,
        Nurbs,
        Dwell,
        MoveType_Max
    };
}


class GLInterp: public InterpBaseClass
{
private:
    struct GLLineStrip
    {
        // Collection of vertices 
        QVector<GLVertex3D> strip;
    };

    struct WizLineLines
    {
        QVector<GLLineStrip> m_strips[MoveType::MoveType_Max];     // Key lines for feed, traverse and arc separate
    };

    GLVertex3D m_vLastPoint;
    MoveType::MoveType m_eLastMoveType;
    int m_nWizLineNo;
    bool m_bHasLastPoint;
    QVector< WizLineLines > m_WizLineGroup;     // Each wizline will have it's own set of lines.
    GLVertex3D m_extentsMin, m_extentsMax, m_extentsMid, m_extentsSize;
    CANON_UNITS m_units;
    
    void SetLastPoint( double x, double y, double z );
    void AppendLine( MoveType::MoveType type, double x, double y, double z );

    // Interp Overrides
    virtual void InitCanon();
    virtual void ArcFeed( int line_number,
                          double first_end, double second_end, double first_axis,
                          double second_axis, int rotation, double axis_end_point,
                          double a_position, double b_position, double c_position,
                          double u_position, double v_position, double w_position );
    virtual void StraightFeed( int line_number,
                               double x, double y, double z,
                               double a, double b, double c,
                               double u, double v, double w );
    virtual void StraightTraverse( int line_number,
                                   double x, double y, double z,
                                   double a, double b, double c,
                                   double u, double v, double w ) ;
    virtual void Dwell(double time) ;
    virtual void Comment(const char *c);
    virtual void NurbsFeed(int line_number, std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k);
    virtual void CanonError(const char *fmt, va_list arg );
    virtual void NextLine(int line_number );
    virtual void UseLengthUnits(CANON_UNITS u);
    virtual CANON_TOOL_TABLE GetExternalToolTable(int pocket);
    
private:
    static void unrotate(double &x, double &y, double c, double s); 
    static void rotate(double &x, double &y, double c, double s); 
    static void EmcPose2Array( const EmcPose &pos, double o[9] );
    void rs274_arc_to_segments( int line_number, double x1, double y1, double cx, double cy, int number_rotations, double z1, double a, double b, double c, double u, double v, double w ); 
    void ComputeSizeAndMid();
    
public:
    GLInterp();
    void parse_file( const char *filename );
    DrawColour::DrawColours ColourIndex( MoveType::MoveType eType, bool bHighlight ) ;
    void DrawRowLines( bool bHighlight, const WizLineLines &wl );
    void DrawLines( int nHighlightRow );
    GLVertex3D GetExtentsMid() { return m_extentsMid; }
    GLVertex3D GetExtentsSize() { return m_extentsSize; }
    GLVertex3D GetExtentsMin() { return m_extentsMin; }
    GLVertex3D GetExtentsMax() { return m_extentsMax; }
    CANON_UNITS GetUnits() const { return m_units; }
};


#endif // GLINTERP_H
