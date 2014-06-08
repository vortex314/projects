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
#define SYS 'Y'
#define EVENT(cls,type) ((cls<<24)+(type<<16))

class Str;
class Sys  {
public:
    enum SysEvents {
        ERROR=EVENT(SYS,'E')
    };
    
    Sys(); // constructor for eventSource
    static void init();
    static void delay_ms(uint32_t msec);
    static uint64_t _upTime;
    static uint64_t upTime();
    static uint64_t _bootTime;
    static void * malloc(uint32_t size);
    static void free(void *pv);
    static void interruptEnable();
    static void interruptDisable();
    static void log(Str& str);
    static void log(const char* line);
    static Str _log;
};

#endif /* SYS_H_ */
