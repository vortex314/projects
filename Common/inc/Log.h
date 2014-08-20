#ifndef LOG_H
#define LOG_H
#include <iostream>

using namespace std;
#include <Str.h>


class Log : public Str {
    public:
        typedef uint64_t EOL;
        EOL eol;
        Log() ;
        void flush();
        Log& dump(Bytes& bytes);
    };


#endif // LOG_H
