#include "Log.h"
#include "Sys.h"
Log::Log() : Str(100) {
    }
Log& Log::flush() {
/*    cout << Sys::upTime()/1000<<"."<< Sys::upTime() %1000 << " | " ;
    offset(0);

    while(hasData())
        std::cout << read();
    cout << std::endl;*/
    clear();
    return *this;
    }
const char *HEX="0123456789ABCDEF";
Log& Log::dump(Bytes& bytes) {
    uint32_t  i;

    for(i=0; i<bytes.length(); i++) {
        uint8_t b;
        b=bytes.read();
        *this << (char)HEX[b>>4]<< (char)HEX[b&0x0F] << " ";
        }
    bytes.offset(0);
    for(i=0; i<bytes.length(); i++) {
        uint8_t b;
        b=bytes.read();
        if ( isprint((char)b)) *this << (char)b;
        else *this << '.';
        }
    return *this;
    }

