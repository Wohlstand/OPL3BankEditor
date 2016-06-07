#ifndef JUNLEVIZION_H
#define JUNLEVIZION_H

#include <QString>
#include "../bank.h"

class JunleVizion
{
public:
    enum ErrCode
    {
        ERR_OK=0,
        ERR_NOFILE,
        ERR_BADFORMAT
    };
    static int loadFile(QString filePath, FmBank &bank);
    static int saveFile(QString filePath, FmBank &bank);
};

#endif // JUNLEVIZION_H
