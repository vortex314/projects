#include "Logger.h"

Logger::Logger()
{
    _logLine = new Str(100);
}

Logger::Logger(uint16_t size) :Str(size)
    {
        _logLine=new Str(size+10);
    };

Logger::~Logger()
{
    //dtor
}
#include <iostream>
void Logger::flush()
    {
        _logLine->clear();
        _logLine->append(Sys::upTime());
        _logLine->append(" : ");
        _logLine->append(this);
        std::cout << _logLine->data() << std::endl;
        clear();
    }
