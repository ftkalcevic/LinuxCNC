#ifndef APPLICATIONSETTINGS_H
#define APPLICATIONSETTINGS_H

#include <QSettings>
#include "glutility.h"

class DrawColour
{
public:
    enum DrawColours
    {
        BackColour,
        FeedColour,
        FeedHighlightColour,
        ArcColour,
        ArcHighlightColour,
        TraverseColour,
        TraverseHighlightColour,
        NurbsColour,
        NurbsHighlightColour,
        DwellColour,
        DwellHighlightColour,
        AxisXColour,
        AxisYColour,
        AxisZColour,
        MAX
    };
};

class ApplicationSettings : public QSettings
{
    Q_OBJECT
public:
    
    explicit ApplicationSettings(const QString &sIniPath, QObject *parent = 0);
    
    const QString &FontPath() const { return m_sFontPath; }
    const QString &CharMapPath() const { return m_sCharMapPath; }
    GLRGBA Colour( DrawColour::DrawColours c ) const { return m_Colours[(int)c]; }
    double MinPhysicalGridSpacing() const { return m_nMinPhysicalGridSpacing; }
    
private:
    QString m_sFontPath;
    QString m_sCharMapPath;
    QVector<GLRGBA> m_Colours;
    double m_nMinPhysicalGridSpacing;
   
signals:
    
public slots:
    
};

#endif // APPLICATIONSETTINGS_H
