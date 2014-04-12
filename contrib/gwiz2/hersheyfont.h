#ifndef HERSHEYFONT_H
#define HERSHEYFONT_H

class HersheyFontBase;

class HersheyFont
{
    static HersheyFontBase *m_pFontBase;
    class HersheyCharacterMap *m_pCharMap;
    
public:
    enum
    {
    } EHersheyFont;
    HersheyFont();
    void plot_string( const QString &s, double frac = 0 ) const;   // optional frac? bounding box
private:
    void CalcFontScale();


    int m_yoffset;
    double m_yscale;
};

#endif // HERSHEYFONT_H
