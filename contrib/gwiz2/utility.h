#ifndef QTUTILITY_H
#define QTUTILITY_H

class Utility
{
private:
    Utility();
public:
    static bool ReadImage(QDir &dir, QString sFileName, QPixmap &image);
    static bool ReadFile(QString sPath, QString &sContents);
    static QString ProcessPercents(QString sGCode);
};

#endif // QTUTILITY_H
