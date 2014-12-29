#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include "BipBuffer.h"
#include "Msg.h"
#include "Event.h"
#include "Signal.h"

typedef struct
{
    uint16_t length;
//	Signal signal;
} Envelope;

BipBuffer Msg::bb;

#define ENVELOPE_SIZE	sizeof(Envelope)

Msg::Msg()
{
    _bufferStart = 0;
    _signal = SIG_IDLE;
}

Msg::Msg(int size)
{
    create(size);
    _signal = SIG_IDLE;
}

Msg::Msg(Signal sig)
{
    _bufferStart = 0;
    _signal = sig;
    map(0, 0);
//	_bufferStart = (uint)&env.signal;
}

Msg& Msg::create(int size)
{
    if (!bb.IsInitialized())
        bb.AllocateBuffer(1024);
    int reserved;
    _bufferStart = bb.Reserve(size + ENVELOPE_SIZE, reserved);
    if (_bufferStart)
        map(_bufferStart + ENVELOPE_SIZE, reserved - ENVELOPE_SIZE);
    else
        Sys::warn(EINVAL, "MSG");
    return *this;
}

Msg& Msg::open()
{
    Envelope env;
    map(0, 0);

    // read length
    int size = 2;
    _bufferStart = bb.GetContiguousBlock(size);

    if (size > 2)   		// map to these bytes
    {
        memcpy(&env, _bufferStart, ENVELOPE_SIZE);
        map(_bufferStart + ENVELOPE_SIZE, env.length - ENVELOPE_SIZE);
        Cbor cbor(*this);
        cbor.get((uint32_t&)_signal);
    }
    else
    {
        _signal = SIG_IDLE;
    }
    // read signal
    return *this;
}

void Msg::recv()
{
    bb.DecommitBlock(length() + ENVELOPE_SIZE);
}

void Msg::send()
{
    Envelope env = { length() + ENVELOPE_SIZE /*,_signal */};
    ::memcpy(_bufferStart, &env, ENVELOPE_SIZE);
    bb.Commit(length() + ENVELOPE_SIZE);
}

Msg& Msg::rewind()
{
    offset(0);
    return *this;
}

bool Msg::isEmpty()
{
    return _signal == SIG_IDLE;
}

void Msg::publish(Signal sig)
{
    Msg msg;
    msg.create(5);
    Cbor cbor(msg);
    cbor.add(sig);
    msg.send();
}

void Msg::publish(Signal sig, uint16_t detail)
{
    Msg msg;
    msg.create(10).sig(sig);
    Cbor cbor(msg);
    cbor.add(sig);
    cbor.add((uint64_t) detail);
    msg.send();
}

void Msg::get(Bytes& bytes)
{
    Cbor cbor(*this);
    cbor.get(bytes);
}

void Msg::get(uint32_t& i)
{
    Cbor cbor(*this);
    cbor.get(i);
}

Msg& Msg::add(Bytes& bytes)
{
    Cbor cbor(*this);
    cbor.add(bytes);
    return *this;
}

Msg& Msg::sig(Signal sig)
{
    _signal = sig;
    Cbor cbor(*this);
    cbor.add(sig);
    return *this;
}

Signal Msg::sig()
{
    return _signal;
}

