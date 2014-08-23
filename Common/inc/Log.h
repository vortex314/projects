#ifndef LOG_H
#define LOG_H
#include <iostream>

using namespace std;
#include <Str.h>


class Log : public Str {
    public:
        static Log& log();
        Log(int size);
        typedef uint64_t EOL;
        EOL eol;
        Log();
        Log& flush();
        Log& dump(Bytes& bytes);
    };


#endif // LOG_H
