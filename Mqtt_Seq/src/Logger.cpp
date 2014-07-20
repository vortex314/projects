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
    std::cout << Sys::upTime() << " | " ;
    offset(0);
    while( hasData() )
        std::cout << read();
    std::cout  << std::endl;
    clear();
}


