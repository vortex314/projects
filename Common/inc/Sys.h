/*
 * Sys.h
 *
 *  Created on: 31-aug.-2013
 *      Author: lieven2
 */

#ifndef SYS_H_
#define SYS_H_
#include "stdint.h"
#include "Erc.h"

class Str;
class Sys;
class Logger;


class Sys {
    public:

        Sys();
        static void init();

        static void delay_ms(uint32_t msec);
        static uint64_t _upTime;
        static uint64_t upTime();
        static uint64_t _bootTime;

        static void * malloc(uint32_t size);
        static void free(void *pv);

        static void interruptEnable();
        static void interruptDisable();

        static Str&  log();
        static void warn(int err,const char* s);
        static Str&  lastLog();
        static Str& logFlush();
        static Str& getDeviceName();

    private :
        static Str _logLine;
        static Str _lastLogLine;
        static uint32_t _errorCount;

    };


#endif /* SYS_H_ */
