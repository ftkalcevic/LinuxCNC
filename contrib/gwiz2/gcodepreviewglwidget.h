#ifndef GCODEPREVIEWGLWIDGET_H
#define GCODEPREVIEWGLWIDGET_H

#include <QtOpenGL/QGLWidget>
#include "glutility.h"
#include "glnavbase.h"
#include "hersheyfont.h"
#include "glinterp.h"


class QtLogo;

class GCodePreviewGLWidget : public QGLWidget, protected GLNavBase
{
    Q_OBJECT
public:
    explicit GCodePreviewGLWidget(bool bLathe, QWidget *parent = 0);
    ~GCodePreviewGLWidget();
    
    void Load(const char *sFilename, int hightlight_line, bool bResetView);
    
    void Init();
    void HighlightLine(int line);
    void SetView( ViewType::ViewTypes v ) { setView(v); updateGL(); }
    void SetDefaultView();
    void SetGrid( bool bGridOn );
    ViewType::ViewTypes GetView() const { return getView(); }
    
public slots:

signals:

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *);

    virtual void invalidateScene() { updateGL(); }
    virtual void drawScene();
    virtual int wininfo_width() { return width(); }
    virtual int wininfo_height() { return height(); }
    virtual GLVertex3D extentsinfo_mid();
    virtual GLVertex3D extentsinfo_size();
    virtual bool is_lathe()  { return m_bLathe; }
    
private:
    bool m_bLathe;
    bool m_bGrid;
    int m_highlightedLine;
    class GLInterp *m_pInterp;
    HersheyFont m_font;
    
    void DrawAxes();
    void DrawGrid();
    void DrawGrid( double ext_min_x, double ext_max_x, double ext_min_y, double ext_max_y );
    void ComputeGridParams(double ext_min, double ext_max, int &min, int &max, int &large_step, int &small_step);
};

#endif // GCODEPREVIEWGLWIDGET_H
