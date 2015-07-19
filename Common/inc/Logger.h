#ifndef LOGGER_H
#define LOGGER_H
#include <time.h>
#include <Str.h>
#include <errno.h>
#include <string.h>
class Logger
{
public :
    enum Level { DEBUG, INFO,WARN,ERROR, FATAL};
    Logger(int size) ;
    Logger& level(int level);
    void setLevel(Level l);
    Logger& module(const char * m);
    Logger& log(const char *s);
    Logger& log(int i);
    Logger& operator<<(const char *s );
    Logger& operator<<(int i );
    Logger& operator<<(Bytes& b);
    Logger& operator<<(Str& str);
    Logger& flush();
    Logger& debug()
    {
        return level(DEBUG);
    };
    Logger& warn()
    {
        return level(WARN);
    };
    Logger& info()
    {
        return level(INFO);
    };
    Logger& error()
    {
        return level(ERROR);
    };
    Logger& fatal()
    {
        return level(FATAL);
    };
    Logger& perror(const char* s)
    {
        level(ERROR);
        _str << s << " failed. errno = "<< errno << ":" << strerror(errno);
        flush();
        return *this;
    };
    const char* logTime();

private :
    int _level;
    int _logLevel;
    const char* _module;
    time_t _time;
    Str _str;
};
#define LOG_FILE __FILE__
#define LOG_DATE __DATE__
#endif // LOGGER_H
