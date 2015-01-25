/*
 * BipBuffer.h
 *
 *  Created on: 12-sep.-2014
 *      Author: lieven2
 */

#ifndef BIPBUFFER_H_
#define BIPBUFFER_H_
#include <stdint.h>
#include <malloc.h>
#include <string.h>


class BipBuffer {
private:
	uint8_t* pBuffer;
	int ixa;
	int sza;
	int ixb;
	int szb;
	int buflen;
	int ixResrv;
	int szResrv;

public:
	BipBuffer() ;
	~BipBuffer() ;
	bool AllocateBuffer(int buffersize);
	void Clear() ;
	void FreeBuffer() ;
	uint8_t* Reserve(int size, int& reserved);
	void Commit(int size);
	uint8_t* GetContiguousBlock(uint32_t& size);
	void DecommitBlock(int size);
	int GetCommittedSize() const ;
	int GetReservationSize() const;
	int GetBufferSize() const;
	bool IsInitialized() const;
private:
	int GetSpaceAfterA() const ;

	int GetBFreeSpace() const ;
};


#endif /* BIPBUFFER_H_ */
