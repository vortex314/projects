#include "Log.h"
#include "Sys.h"
Log::Log() : Str(100) {
    }
#define isprint(c) (((c)>0x1F) && ((c)>0x7f))
Log& Log::flush() {
/*    cout << Sys::upTime()/1000<<"."<< Sys::upTime() %1000 << " | " ;
    offset(0);

    while(hasData())
        std::cout << read();
    cout << std::endl;*/
    clear();
    return *this;
    }
Log& Log::dump(Bytes& bytes) {
    uint32_t  i;

    for(i=0; i<bytes.length(); i++) {
        uint8_t b;
        b=bytes.read();
        appendHex(b);
        *this << " ";
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

