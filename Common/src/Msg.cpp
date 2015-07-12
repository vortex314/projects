#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include "BipBuffer.h"
#include "Msg.h"

typedef struct {
	uint16_t length;
} Envelope;

BipBuffer MsgQueue::bb;

#define ENVELOPE_SIZE	sizeof(Msg)

bool Msg::is(Handler* src, int sigMask, int param, void* data) {
	if (sigMask & this->signal) {
		if (src == 0 || src == this->src) {
			if (param == 0 || param == this->param) {
				if (data == 0 || data == this->data)
					return true;
			}
		}
	}
	return false;
}

bool Msg::is(Handler* src, int sigMask) {
	if (src == 0 || src == this->src) {
		if ((sigMask & this->signal) || (sigMask == 0))
			return true;
	}
	return false;
}

bool Msg::is(Handler* src, Signal signal) {
	if (signal == this->signal) {
		if (src == 0 || src == this->src) {
			return true;
		}
	}
	return false;
}

Signal Msg::sig() {
	return this->signal;
}

void MsgQueue::publish(Handler* src, Signal signal, int param, void* data) {
	Msg msg = { src, signal, param, data };
	publish(msg);
}

void MsgQueue::publish(Handler* src, Signal signal) {
	Msg msg = { src, signal, 0, 0 };
	publish(msg);
}

void MsgQueue::publish(Msg& msg) {
	uint8_t* _bufferStart;
	if (!bb.IsInitialized())
		bb.AllocateBuffer(1024);
	int reserved;
	_bufferStart = bb.Reserve(sizeof(Msg), reserved);
	if (_bufferStart)
		memcpy(_bufferStart, &msg, sizeof(Msg));
	else
		Sys::warn(EINVAL, "MSG");
	bb.Commit(sizeof(Msg));
}

bool MsgQueue::get(Msg& msg) {
	uint8_t* _bufferStart;
	uint32_t size = sizeof(Msg);
	_bufferStart = bb.GetContiguousBlock(size);

	if (size < sizeof(Msg))   		// map to these bytes
			{
		msg.signal = SIG_IDLE;
		return false;
	} else {
		memcpy(&msg, _bufferStart, sizeof(Msg));
		bb.DecommitBlock(sizeof(Msg));
		return true;
	}
}

