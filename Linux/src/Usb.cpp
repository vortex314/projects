#include "Usb.h"
const int Usb::RXD=Event::nextEventId("USB::RXD");
const int Usb::ERROR=Event::nextEventId("USB::ERROR");
const int Usb::MESSAGE=Event::nextEventId("USB::MESSAGE");
const int Usb::FREE=Event::nextEventId("USB::FREE");
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include "Logger.h"
static Logger logger(256);

typedef struct
{
    uint32_t baudrate;
    uint32_t symbol;
} BAUDRATE;
BAUDRATE BAUDRATES[]=
{
    {50,  B50	},
    {75, B75	},
    {110,  B110	},
    {134,  B134	},
    {150,  B150	},
    {200,  B200	},
    {300,  B300	},
    {600,  B600	},
    {1200,  B1200	},
    {1800,  B1800	},
    {2400,  B2400	},
    {4800,  B4800	},
    {9600,  B9600	},
    {19200,  B19200	},
    {38400,  B38400	},
    {57600,  B57600   },
    {115200,  B115200  },
    {230400,  B230400  },
    {460800,  B460800  },
    {500000,  B500000  },
    {576000,  B576000  },
    {921600,  B921600  },
    {1000000,  B1000000 },
    {1152000,  B1152000 },
    {1500000,  B1500000 },
    {2000000,  B2000000 },
    {2500000,  B2500000 },
    {3000000,  B3000000 },
    {3500000,  B3500000 },
    {4000000,  B4000000 }
};

Usb::Usb(const char* device) : msg(100),inBuffer(256)
{
    logger.module("Usb");
    _device =  device;
    isConnected(false);
    _fd=0;
    PT_INIT(&t);
    _isComplete = false;
}

void Usb::setDevice(const char* device)
{
    _device =  device;
}

void Usb::setBaudrate(uint32_t baudrate)
{
    _baudrate =  baudrate;
}

uint32_t baudSymbol(uint32_t value)
{
    for(uint32_t i=0; i<sizeof(BAUDRATES)/sizeof(BAUDRATE); i++)
        if ( BAUDRATES[i].baudrate == value) return BAUDRATES[i].symbol;
    logger.level(Logger::INFO)<< "connect: baudrate " << value << " not found, default to 115200.";
    logger.flush();
    return B115200;
}

Erc Usb::connect()
{
    struct termios options;
    logger.info() << "Connecting to " << _device  << " ...";
    logger.flush();

    _fd = ::open(_device, O_RDWR | O_NOCTTY | O_NDELAY);

    if (_fd == -1)
    {
        logger.error() <<  "connect: Unable to open " << _device << " : "<< strerror(errno);
        logger.flush();
        return E_AGAIN;
    }
    fcntl(_fd, F_SETFL, FNDELAY);

    tcgetattr(_fd, &options);
    cfsetispeed(&options, baudSymbol(_baudrate));
    cfsetospeed(&options, baudSymbol(_baudrate));


    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |=  CS8;
    options.c_cflag &= ~CRTSCTS;               /* Disable hardware flow control */


    options.c_lflag &= ~(ICANON | ECHO | ISIG);


    tcsetattr(_fd, TCSANOW, &options);
    logger.level(Logger::INFO)<<"set baudrate to "<<_baudrate;
    logger.flush();
    logger.level(Logger::INFO) << "open " << _device << " succeeded.";
    logger.flush();
    isConnected(true);
    return E_OK;
}
#include <linux/serial.h>
#include <sys/ioctl.h>
void Usb::logStats()
{
    struct serial_icounter_struct counters;
    int rc = ioctl(_fd, TIOCGICOUNT, &counters);
    if ( rc < 0 )
    {
        logger.error().perror("ioctl TIOCGICOUNT failed").flush();
        return;
    }
    logger.info() << " overrun : " << counters.buf_overrun;
}

Erc Usb::disconnect()
{
    isConnected(false);
    _isComplete=false;
    if ( ::close(_fd) < 0 )   return errno;
    return E_OK;
}

Erc Usb::send(Bytes& bytes)
{
    logger.debug()<< "send() " << bytes;
    logger.flush();
    bytes.AddCrc();
    bytes.Encode();
    bytes.Frame();
    logger.level(Logger::DEBUG)<<"send full : "<<bytes;
    logger.flush();
    uint32_t count = ::write(_fd,bytes.data(),bytes.length());
    if ( count != bytes.length())
    {
        disconnect();
        logger.perror("send() failed !");
        logger.flush();
        return E_AGAIN;
    }
    return E_OK;
}

uint8_t Usb::read()
{
    uint8_t b;
    if (::read(_fd,&b,1)<0)
    {
        logger.level(Logger::WARN).perror("read() failed : ");
        logger.flush();
        return 0;
    }
//   fprintf(stdout,".");
//   fflush(stdout);
    return b;
}

#include <sys/ioctl.h>
uint32_t Usb::hasData()
{
    int count;
    int rc = ioctl(_fd, FIONREAD, (char *) &count);
    if (rc < 0)
    {
        logger.level(Logger::WARN).perror("ioctl() ");
        logger.flush();
        isConnected(false);
        return 0;
    }
    return count;
}

int Usb::handler ( Event* event )
{
    uint8_t b;
    uint32_t i;
    uint32_t count;

    if ( event->is(Usb::ERROR ))
    {
        logger.level(Logger::WARN) << " error occured. Reconnecting.";
        logger.flush();
        disconnect();
        connect();
        return 0;
    }
    PT_BEGIN ( &t );
    while(true)
    {
        while( isConnected())
        {
            PT_YIELD_UNTIL(&t,event->is(RXD) || event->is(FREE) || ( inBuffer.hasData() && (_isComplete==false)) );
            if ( event->is(RXD) &&  hasData())
            {
                count =hasData();
                for(i=0; i<count; i++)
                {
                    b=read();
                    inBuffer.write(b);
                }
            }
            else if ( event->is(FREE))   // re-use buffer after message handled
            {
                _isComplete=false;
                msg.clear();
            };
            if ( inBuffer.hasData() && (_isComplete==false) )
            {
                while( inBuffer.hasData() )
                {
                    if ( msg.Feed(inBuffer.read()))
                    {
                        logger.level(Logger::DEBUG)<< "recv : " << msg;
                        logger.flush();
                        msg.Decode();
                        if ( msg.isGoodCrc() )
                        {
                            msg.RemoveCrc();
                            logger.level(Logger::INFO)<<"-> TCP : " <<msg;
                            logger.flush();
                            publish(MESSAGE);
                            publish(FREE); // re-use buffer after message handled
                            _isComplete=true;
                            break;
                        }
                        else
                        {
                            logger.level(Logger::WARN)<<"Bad CRC. Dropped packet. ";
                            logger.flush();
 //                           logStats();
                            msg.isGoodCrc();
                            msg.clear(); // throw away bad data
                        }
                    }
                }
            }
            PT_YIELD ( &t );
        }
        PT_YIELD_UNTIL ( &t,isConnected() );
    }

    PT_END ( &t );
}

MqttIn* Usb::recv()
{
    if ( _isComplete )
    {
        return &msg;
    }
    return 0;
}

int Usb::fd()
{
    return _fd;
}






