#ifndef WIZLINETEXT_H
#define WIZLINETEXT_H

#include "wizline.h"

class WizLineText : public WizLine
{
public:
    WizLineText(const QString &sLine);
    virtual WizLine *Clone() const;
};

#endif // WIZLINETEXT_H
