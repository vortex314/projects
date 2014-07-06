#ifndef LOGGER_H
#define LOGGER_H
#include "Str.h"

class Logger : public Str
{
public:
    Logger();
    virtual ~Logger();

    Logger(uint16_t size) ;

    void flush();
protected:
private:
    Str* _logLine;
};

#endif // LOGGER_H
