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

#include <iostream>
#define EVENT(cls,type) ((cls<<24)+(type<<16))

class Str;
class Sys;
class Logger;


class Sys
{
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

    static void logger(const char* s);
    static void logger(Str& str);
    static void flushLogger();

    static Logger&  getLogger();
    static char* getDeviceName();
    static Logger* _logger;

};


#endif /* SYS_H_ */
