#ifndef GLNAVBASE_H
#define GLNAVBASE_H

#include "glutility.h"

namespace ViewType
{
    enum ViewTypes
    {
        X,
        Y,
        Z,
        Z2,
        P
    };
}
class GLNavBase
{
public:
    GLNavBase();
    void set_background(float r, float g, float b);
    void set_centerpoint(double x, double y, double z);
    void set_latitudelimits(double new_minlat, double new_maxlat);
    void set_eyepoint(double new_distance);
    void set_eyepoint_from_extents(double e1, double e2);
    void reset();
    void recordMouse(int x, int y);
    void startRotate(int x, int y);
    void scale(int x, int y);
    void rotate(int x, int y);
    void translate(int x, int y);
    void set_viewangle(double new_lat, double new_lon);
    void zoomin();
    void zoomout();
    void startZoom(int y);
    void continueZoom(int y);
    bool getRotateMode();
    void translateOrRotate(int x, int y);
    void rotateOrTranslate(int x, int y);
    void setView( ViewType::ViewTypes v );
    ViewType::ViewTypes getView() const { return view; }
    
    virtual void invalidateScene() = 0;
    virtual void drawScene() = 0;
    virtual int wininfo_width() = 0;
    virtual int wininfo_height() = 0;
    virtual GLVertex3D extentsinfo_mid() = 0;
    virtual GLVertex3D extentsinfo_size() = 0;
    virtual bool is_lathe() = 0;
    void redrawScene();

protected:
    void redraw_perspective();
    void redraw_ortho();
    void set_view_x();
    void set_view_y();
    void set_view_z();
    void set_view_z2();
    void set_view_p();
    
    int xmouse;         // Current coordinates of the mouse.
    int ymouse;
    double xcenter;      // Where we are centering.
    double ycenter;
    double zcenter;
    float r_back;       // The _back color
    float g_back;
    float b_back;
    double distance;     // Where the eye is
    double fovy;         // Field of view in y direction
    double near;         // Position of clipping planes.
    double far;
    bool perspective;   // View settings
    double lat;
    double lon;
    double minlat;
    double maxlat;
    int y0;             // For zooming
    double original_zoom;
    ViewType::ViewTypes view;
    
    void glTranslateScene(double scale, int x, int y, int mousex, int mousey);
    void glRotateScene(double scale, double xcenter, double ycenter, double zcenter, int x, int y, int mousex, int mousey);
};

#endif // GLNAVBASE_H
