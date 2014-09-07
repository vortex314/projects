#include "Log.h"


#include <iostream>

Str _lastLogLine(256);
Log _SysLog(256);

Log& Log::log() {
    return _SysLog;
    }

Log::Log(int size):Str(size) {
    }

Log& Log::flush() {
    _lastLogLine.clear() << Sys::upTime() << " | " ;
    offset(0);
    _lastLogLine << *this;
    _lastLogLine.offset(0);
    while(_lastLogLine.hasData())
        std::cout << _lastLogLine.read();
    std::cout  <<   std::endl;
    clear();
    return *this;
    }


const char *HEX="0123456789ABCDEF";
Log& Log::dump(Bytes& bytes) {
    bytes.offset(0);
    while(bytes.hasData()) {
        uint8_t b;
        b=bytes.read();
        *this << (char)HEX[b>>4]<< (char)HEX[b&0x0F] << " ";
        }
    bytes.offset(0);
    while(bytes.hasData()) {
        uint8_t b;
        b=bytes.read();
        if ( isprint((char)b)) *this << (char)b;
        else *this << '.';
        }
    return *this;
    }

    Log& Log::message(const char *header , Bytes& bytes){
        bytes.offset(0);
        append(header);
        dump(bytes);
        flush();
    }
