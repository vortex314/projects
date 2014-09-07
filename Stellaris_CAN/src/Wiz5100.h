/*
 * Wiz5100.h
 *
 *  Created on: 20-dec.-2013
 *      Author: lieven2
 */

#ifndef WIZ5100_H_
#define WIZ5100_H_

#include "Spi.h"
#include <stdint.h>
#include "Erc.h"
#include "Stream.h"
#include "Sys.h"
#include "Timer.h"

#define WIZ 'W'

class Wiz5100 : public Stream {
public:

    enum WizEvents {
        CONNECT = EVENT(WIZ, 'C'),
        DISCONNECT = EVENT(WIZ, 'D'),
        RXD = EVENT(WIZ, 'R'),
        TXD = EVENT(WIZ, 'T'),
        TIMEOUT = EVENT(WIZ, 't'),
        INTERRUPT = EVENT(WIZ, 'I'),
        STATUS_CHANGE = EVENT(WIZ, 'S')
    };

    Wiz5100(uint32_t socket);
    ~Wiz5100();
    int init();
     void reset();
     Erc loadCommon(uint8_t mac[6], uint8_t ipAddress[4], uint8_t gtwAddr[4], uint8_t netmask[4]);

     int write(uint16_t address, uint8_t data);
     int write16(uint16_t address, uint16_t data);
     int read(uint16_t address, uint8_t* data);
     int read16(uint16_t address, uint16_t* data);
    int socketCmd(uint8_t cmd);
    uint8_t getSR();
    void setCR(uint8_t cmd);
    uint8_t getCR();
    uint8_t getIR();
//    void setIR(uint8_t);
    int socket();
    void poll();
    Erc event(Event& event);
private:
    uint32_t _socket;
    uint16_t _socketAddr;
    uint8_t SR;
    uint8_t IR;
    uint16_t _ipPort;
    Timer _timer;
    Spi _spi;
};

/*
 *  Generic status codes, not associated with the Wiznet W5100 device.
 */
#define  W5100_OK				0x00		/* success */
#define  W5100_FAIL				0xff		/* generic failure code */
/*
 *  Wiznet W5100 opcodes
 */
#define W5100_WRITE_OPCODE			0xF0
#define W5100_READ_OPCODE			0x0F
/*
 *  Wiznet W5100 register addresses
 */
#define  W5100_MR   			0x0000   	/* Mode Register */
#define  W5100_GAR  			0x0001   	/* Gateway Address: 0x0001 to 0x0004 */
#define  W5100_SUBR 			0x0005   	/* Subnet mask Address: 0x0005 to 0x0008 */
#define  W5100_SHAR  			0x0009   	/* Source Hardware Address (MAC): 0x0009 to 0x000E */
#define  W5100_SIPR 			0x000F   	/* Source IP Address: 0x000F to 0x0012 */
#define  W5100_IR				0x0015		/* Interrupt Register */
#define  W5100_IMR				0x0016		/* Interrupt Mask Register */
#define  W5100_RTR				0x0017		/* Retry Timeout Register: 0x0017 to 0x0018 */
#define  W5100_RCR				0x0019		/* Retry Count Register */
#define  W5100_RMSR 			0x001A   	/* RX Memory Size Register */
#define  W5100_TMSR 			0x001B   	/* TX Memory Size Register */
#define  W5100_PATR				0x001C		/* PPPoE Authentication-Type Register: 0x001C to 0x001D */
#define  W5100_PTIMER			0x0028		/* PPP Link Control Protocol Request Timer Register */
#define  W5100_PMAGIC			0x0029		/* PPP Link Control Protocol Magic number Register */
#define  W5100_UIPR				0x002A		/* Unreachable IP Address Register: 0x002A to 0x002D */
#define  W5100_UPORT			0x002E		/* Unreachable Port Register: 0x002E to 0x002F */
/*
 *  Use W5100_SKT_REG_BASE and W5100_SKT_OFFSET to calc the address of each set of socket registers.
 *  For example, the base address of the socket registers for socket 2 would be:
 *  (W5100_SKT_REG_BASE + (2 * W5100_SKT_OFFSET))
 */
#define  W5100_SKT_REG_BASE		0x0400		/* start of socket registers */
#define  W5100_SKT_OFFSET		0x0100		/* offset to each socket regester set */
#define  W5100_SKT_BASE(n)		(W5100_SKT_REG_BASE+(n*W5100_SKT_OFFSET))
#define  W5100_NUM_SOCKETS		4
/*
 *  Define the addresses inside the W5100 for the transmit and receive
 *  buffers.
 */
#define  W5100_TXBUFADDR		0x4000      /* W5100 Send Buffer Base Address */
#define  W5100_RXBUFADDR		0x6000      /* W5100 Read Buffer Base Address */

/*
 *  Define masks for accessing the addresses with a TX or RX buffer.
 *  Note that these masks assume 2K buffers!
 */
#define  W5100_TX_BUF_MASK      0x07FF		/* Tx 2K Buffer Mask */
#define  W5100_RX_BUF_MASK      0x07FF		/* Rx 2K Buffer Mask */
/*
 *  The following offsets are added to a socket's base register address to find
 *  a specific socket register.  For example, the address of the command register
 *  for socket 2 would be:
 *  (W5100_SKT_BASE(2) + W5100_CR_OFFSET
 */
#define  W5100_MR_OFFSET		0x0000		/* socket Mode Register offset */
#define  W5100_CR_OFFSET		0x0001		/* socket Command Register offset */
#define  W5100_IR_OFFSET		0x0002		/* socket Interrupt Register offset */
#define  W5100_SR_OFFSET		0x0003		/* socket Status Register offset */
#define  W5100_PORT_OFFSET		0x0004		/* socket Port Register offset (2 bytes) */
#define  W5100_DHAR_OFFSET		0x0006		/* socket Destination Hardware Address Register (MAC, 6 bytes) */
#define  W5100_DIPR_OFFSET		0x000C		/* socket Destination IP Address Register (IP, 4 bytes) */
#define  W5100_DPORT_OFFSET		0x0010		/* socket Destination Port Register (2 bytes) */
#define  W5100_MSS_OFFSET		0x0012		/* socket Maximum Segment Size (2 bytes) */
#define  W5100_PROTO_OFFSET		0x0014		/* socket IP Protocol Register */
#define  W5100_TOS_OFFSET		0x0015		/* socket Type Of Service Register */
#define  W5100_TTL_OFFSET		0x0016		/* socket Time To Live Register */
#define  W5100_TX_FSR_OFFSET	0x0020		/* socket Transmit Free Size Register (2 bytes) */
#define  W5100_TX_RR_OFFSET		0x0022		/* socket Transmit Read Pointer Register (2 bytes) */
#define  W5100_TX_WR_OFFSET		0x0024		/* socket Transmit Write Pointer Register (2 bytes) */
#define  W5100_RX_RSR_OFFSET	0x0026		/* socket Receive Received Size Register (2 bytes) */
#define  W5100_RX_RD_OFFSET		0x0028		/* socket Receive Read Pointer Register (2 bytes) */

#define Sx_MR(n)       (0x0400+(n*0x100))              /* socket Mode Register offset */
#define Sx_CR(n)       (0x0401+(n*0x100))              /* socket Command Register offset */
#define  Sx_IR(n)       (0x0402+(n*0x100))		/* socket Interrupt Register offset */
#define  Sx_SR(n)      (0x0403+(n*0x100))		/* socket Status Register offset */
#define  Sx_PORT(n)    (0x0404+(n*0x100))		/* socket Port Register offset (2 bytes) */
#define  Sx_DHAR (n)   (0x0406+(n*0x100))		/* socket Destination Hardware Address Register (MAC, 6 bytes) */
#define  Sx_DIPR(n)    (0x040C+(n*0x100))		/* socket Destination IP Address Register (IP, 4 bytes) */
#define  Sx_DPORT(n)   (0x0410+(n*0x100))		/* socket Destination Port Register (2 bytes) */
#define  Sx_MSS(n)     (0x0412+(n*0x100))		/* socket Maximum Segment Size (2 bytes) */
#define  Sx_PROTO(n)   (0x0414+(n*0x100))		/* socket IP Protocol Register */
#define  Sx_TOS(n)     (0x0415+(n*0x100))		/* socket Type Of Service Register */
#define  Sx_TTL(n)     (0x0416+(n*0x100))		/* socket Time To Live Register */
#define  Sx_TX_FSR(n)  (0x0420+(n*0x100))		/* socket Transmit Free Size Register (2 bytes) */
#define  Sx_TX_RR(n)   (0x0422+(n*0x100)		/* socket Transmit Read Pointer Register (2 bytes) */
#define  Sx_TX_WR(n)   (0x0424+(n*0x100))		/* socket Transmit Write Pointer Register (2 bytes) */
#define  Sx_RX_RSR(n)  (0x0426+(n*0x100))		/* socket Receive Received Size Register (2 bytes) */
#define  Sx_RX_RD(n)   (0x0428+(n*0x100))		/* socket Receive Read Pointer Register (2 bytes) */
#define Sx_TX_BASE(n) (0x4000+n*0x800)   // 2K buffers
#define Sx_RX_BASE(n) (0x6000+n*0x800)   // 2K buffers

#define S_MR       (0x0000)              /* socket Mode Register offset */
#define S_CR       (0x0001)              /* socket Command Register offset */
#define  S_IR       (0x0002)		/* socket Interrupt Register offset */
#define  S_SR      (0x0003)		/* socket Status Register offset */
#define  S_PORT    (0x0004)		/* socket Port Register offset (2 bytes) */
#define  S_DHAR    (0x0006)		/* socket Destination Hardware Address Register (MAC, 6 bytes) */
#define  S_DIPR   (0x000C)		/* socket Destination IP Address Register (IP, 4 bytes) */
#define  S_DPORT   (0x0010)		/* socket Destination Port Register (2 bytes) */
#define  S_MSS     (0x0012)		/* socket Maximum Segment Size (2 bytes) */
#define  S_PROTO   (0x0014)		/* socket IP Protocol Register */
#define  S_TOS     (0x0015)		/* socket Type Of Service Register */
#define  S_TTL     (0x0016)		/* socket Time To Live Register */
#define  S_TX_FSR  (0x0020))		/* socket Transmit Free Size Register (2 bytes) */
#define  S_TX_RR   (0x0022)		/* socket Transmit Read Pointer Register (2 bytes) */
#define  S_TX_WR   (0x0024)		/* socket Transmit Write Pointer Register (2 bytes) */
#define  S_RX_RSR  (0x0026)		/* socket Receive Received Size Register (2 bytes) */
#define  S_RX_RD   (0x0028)		/* socket Receive Read Pointer Register (2 bytes) */
/*
 *  The following #defines are OR-masks for creating mode values to write
 *  to the Mode register.  Note that these values are only valid when writing
 *  to the chip's Mode register.  These values are NOT to be used with the
 *  individual socket Mode registers.
 */
#define  W5100_MR_SOFTRST		(1<<7)		/* soft-reset */
#define  W5100_MR_PINGBLK		(1<<4)		/* block responses to ping request */
#define  W5100_MR_PPPOE			(1<<3)		/* enable PPPoE */
#define  W5100_MR_AUTOINC		(1<<1)		/* address autoincrement (indirect interface ONLY!) */
#define  W5100_MR_INDINT		(1<<0)		/* use indirect interface (parallel interface ONLY!) */
/*
 *  The following #defines are OR-masks for checking bits in the Interrupr Register,
 *  used by the host MCU to determine the current state of the W5100.  Note that these
 *  values are only valid with writing to the chip's Interrupt register.  These values
 *  are NOT to be used with the individual's socket Interrupt registers.
 */
#define  W5100_IR_CONFLICT		(1<<7)		/* 1 means ARP request received with this machine's IP */
#define  W5100_IR_UNREACH		(1<<6)		/* 1 means IP addr & port were unreachable (ICMP packet received) */
#define  W5100_IR_PPPOE			(1<<5)		/* 1 means PPPoE connection closed */
#define  W5100_IR_S3_INT		(1<<3)		/* 1 means socket 3 interrup occurred */
#define  W5100_IR_S2_INT		(1<<3)		/* 1 means socket 2 interrup occurred */
#define  W5100_IR_S1_INT		(1<<3)		/* 1 means socket 1 interrup occurred */
#define  W5100_IR_S0_INT		(1<<3)		/* 1 means socket 0 interrup occurred */
/*
 *  The following #defines are OR-masks for changing bits in the Interrupt Mask Register.
 *  Setting a mask bit ENABLES the associated interrupt.
 *  (I have changed the names from the originals used by Wiznet; they labeled bits as
 *  IM_IRx, which isn't very helpful.)
 */
#define  W5100_IMR_CONFLICT		(1<<7)		/* Enable interrupt for ARP request received with this machine's IP */
#define  W5100_IMR_UNREACH		(1<<6)		/* Enable interrupt for IP addr & port were unreachable (ICMP packet received) */
#define  W5100_IMR_PPPOE		(1<<5)		/* Enable interrupt for PPPoE connection closed */
#define  W5100_IMR_S3_INT		(1<<3)		/* Enable interrupt for socket 3 interrup */
#define  W5100_IMR_S2_INT		(1<<3)		/* Enable interrupt for socket 2 interrup */
#define  W5100_IMR_S1_INT		(1<<3)		/* Enable interrupt for socket 1 interrup */
#define  W5100_IMR_S0_INT		(1<<3)		/* Enable interrupt for socket 0 interrup */
/*
 *  The following #defines are OR-masks for selecting features in the Mode registers for
 *  each of the sockets.  These values are NOT to be used with the chip's Mode register.
 */
#define  W5100_SKT_P3			(1<<3)		/* protocol selecter bit 3 */
#define  W5100_SKT_P2			(1<<2)		/* protocol selecter bit 2 */
#define  W5100_SKT_P1			(1<<1)		/* protocol selecter bit 1 */
#define  W5100_SKT_P0			(1<<0)		/* protocol selecter bit 0 */
/*
 *  The following #defines are values that can be read from the Status registers for
 *  each of the sockets.
 */
#define  W5100_SKT_SR_CLOSED      0x00		/* closed */
#define  W5100_SKT_SR_INIT        0x13		/* init state */
#define  W5100_SKT_SR_LISTEN      0x14		/* listen state */
#define  W5100_SKT_SR_SYNSENT     0x15		/* connection state */
#define  W5100_SKT_SR_SYNRECV     0x16		/* connection state */
#define  W5100_SKT_SR_ESTABLISHED 0x17		/* success to connect */
#define  W5100_SKT_SR_FIN_WAIT    0x18		/* closing state */
#define  W5100_SKT_SR_CLOSING     0x1A		/* closing state */
#define  W5100_SKT_SR_TIME_WAIT	  0x1B		/* closing state */
#define  W5100_SKT_SR_CLOSE_WAIT  0x1C		/* closing state */
#define  W5100_SKT_SR_LAST_ACK    0x1D		/* closing state */
#define  W5100_SKT_SR_UDP         0x22		/* UDP socket */
#define  W5100_SKT_SR_IPRAW       0x32		/* IP raw mode socket */
#define  W5100_SKT_SR_MACRAW      0x42		/* MAC raw mode socket */
#define  W5100_SKT_SR_PPPOE       0x5F		/* PPPOE socket */
/*
 *  The following #defines are values for selecting commands in the Command registers for
 *  each of the sockets.
 */
#define  W5100_SKT_CR_OPEN		0x01		/* open the socket */
#define  W5100_SKT_CR_LISTEN	0x02		/* wait for TCP connection (server mode) */
#define  W5100_SKT_CR_CONNECT	0x04		/* listen for TCP connection (client mode) */
#define  W5100_SKT_CR_DISCON	0x08		/* close TCP connection */
#define  W5100_SKT_CR_CLOSE		0x10		/* mark socket as closed (does not close TCP connection) */
#define  W5100_SKT_CR_SEND		0x20		/* transmit data in TX buffer */
#define  W5100_SKT_CR_SEND_MAC	0x21		/* SEND, but uses destination MAC address (UDP only) */
#define  W5100_SKT_CR_SEND_KEEP	0x22		/* SEND, but sends 1-byte packet for keep-alive (TCP only) */
#define  W5100_SKT_CR_RECV		0x40		/* receive data into RX buffer */

#define W5100_SKT_IR_CON    0x1
#define W5100_SKT_IR_DISCON    0x2
#define W5100_SKT_IR_RECV    0x4
#define W5100_SKT_IR_TIMEOUT    0x8
#define W5100_SKT_IR_SEND_OK    0x10
/*
 *  The following #defines are values written to a socket's Mode register to select
 *  the protocol and other operating characteristics.
 */
#define  W5100_SKT_MR_CLOSE	  	0x00    	/* Unused socket */
#define  W5100_SKT_MR_TCP		0x01    	/* TCP */
#define  W5100_SKT_MR_UDP		0x02    	/* UDP */
#define  W5100_SKT_MR_IPRAW	  	0x03	  	/* IP LAYER RAW SOCK */
#define  W5100_SKT_MR_MACRAW	0x04	  	/* MAC LAYER RAW SOCK */
#define  W5100_SKT_MR_PPPOE	  	0x05	  	/* PPPoE */
#define  W5100_SKT_MR_ND		0x20	  	/* No Delayed Ack(TCP) flag */
#define  W5100_SKT_MR_MULTI	  	0x80	  	/* support multicasting */

/*

class Wiz5100 {
public:
	Wiz5100();
	void init();
	void reset();
	virtual ~Wiz5100();
private :
	Spi _spi;
};*/

#endif /* WIZ5100_H_ */
