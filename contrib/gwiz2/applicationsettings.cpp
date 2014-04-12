#include "stdafx.h"
#include "applicationsettings.h"

ApplicationSettings::ApplicationSettings(const QString &sIniFilePath, QObject *parent) :
    QSettings(sIniFilePath, QSettings::IniFormat, parent)
{
    m_sFontPath = value( "Settings/HERSHEY_FONT_PATH").toString();
    m_sCharMapPath = value( "Settings/HERSHEY_CHARACTER_MAP").toString();
    m_nMinPhysicalGridSpacing = value("Settings/MinPhysicalGridSpacing").toDouble();
    
    m_Colours.fill( GLRGBA(0.5,0.5,0.5,1.0), DrawColour::MAX );
    
    m_Colours[DrawColour::BackColour]                = GLRGBA( value("Colours/BackColour").toString() );                   
    m_Colours[DrawColour::FeedColour]                = GLRGBA( value("Colours/FeedColour").toString() );                   
    m_Colours[DrawColour::FeedHighlightColour]       = GLRGBA( value("Colours/FeedHighlightColour").toString() );          
    m_Colours[DrawColour::ArcColour]                 = GLRGBA( value("Colours/ArcColour").toString() );                    
    m_Colours[DrawColour::ArcHighlightColour]        = GLRGBA( value("Colours/ArcHighlightColour").toString() );           
    m_Colours[DrawColour::TraverseColour]            = GLRGBA( value("Colours/TraverseColour").toString() );               
    m_Colours[DrawColour::TraverseHighlightColour]   = GLRGBA( value("Colours/TraverseHighlightColour").toString() );      
    m_Colours[DrawColour::NurbsColour]               = GLRGBA( value("Colours/NurbsColour").toString() );               
    m_Colours[DrawColour::NurbsHighlightColour]      = GLRGBA( value("Colours/NurbsHighlightColour").toString() );      
    m_Colours[DrawColour::DwellColour]               = GLRGBA( value("Colours/DwellColour").toString() );               
    m_Colours[DrawColour::DwellHighlightColour]      = GLRGBA( value("Colours/DwellHighlightColour").toString() );      
    m_Colours[DrawColour::AxisXColour]               = GLRGBA( value("Colours/AxisXColour").toString() );                  
    m_Colours[DrawColour::AxisYColour]               = GLRGBA( value("Colours/AxisYColour").toString() );                  
    m_Colours[DrawColour::AxisZColour]               = GLRGBA( value("Colours/AxisZColour").toString() );                  
}
