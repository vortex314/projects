#include "Logger.h"
#include <iostream>
#include <sys/time.h>
#include <stdio.h>

const char *sLevel[]={"DEBUG","INFO ","WARN ","ERROR","FATAL"};

Logger::Logger(int size) : _str(size)
{
    _module ="";
}
Logger& Logger::level(int level)
{
    time(&_time);
    _level = level;
    return *this;
}
Logger& Logger::module(const char * m)
{
    _module = m;
    return *this;
}
Logger& Logger::log(const char *s)
{
    _str.append(s);
    return *this;
}
Logger& Logger::log(int i)
{
    _str.append(i);
    return *this;
}
Logger& Logger::operator<<(const char *s )
{
    return log(s);
}
Logger& Logger::operator<<(Str& str )
{
    str.offset(0);
    while(str.hasData()) _str.write(str.read());
    return *this;
}
Logger& Logger::operator<<(int i )
{
    return log(i);
}
Logger& Logger::flush()
{
    std::cout << logTime() ;
    std::cout << " | ";
    std::cout<< sLevel[_level]<< " | ";
    std::cout<< _module << " - ";
    _str.offset(0);
    while(_str.hasData())
        std::cout << _str.read();
    std::cout  <<   std::endl;
    _str.clear();
    _level = INFO;
    return *this;
}
const char* Logger::logTime()
{
    static char line[100];
    struct timeval tv;
    struct timezone tz;
    struct tm *tm;
    gettimeofday(&tv,&tz);
    tm=localtime(&tv.tv_sec);
    sprintf(line,"%d/%02d/%02d %02d:%02d:%02d.%03ld"
            ,tm->tm_year+1900
            ,tm->tm_mon+1
            ,tm->tm_mday
            , tm->tm_hour
            , tm->tm_min
            , tm->tm_sec
            , tv.tv_usec/1000);

//   strftime (line, sizeof(line), "%Y-%m-%d %H:%M:%S.mmm", sTm);
    return line;
}

const char *Hex="0123456789ABCDEF";
Logger& Logger::operator<<(Bytes& bytes) {
    bytes.offset(0);
    while(bytes.hasData()) {
        uint8_t b;
        b=bytes.read();
        _str << (char)Hex[b>>4]<< (char)Hex[b&0x0F] << " ";
        }
    bytes.offset(0);
    while(bytes.hasData()) {
        uint8_t b;
        b=bytes.read();
        if ( isprint((char)b)) _str << (char)b;
        else _str << '.';
        }
    return *this;
    }
