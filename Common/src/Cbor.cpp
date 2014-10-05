#include "Cbor.h"

#include <string.h>
#include <stdio.h>
#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define logger(line) fprintf(stderr, "%s:%d [%s]: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, line)
#define loggerf(format, ...) fprintf(stderr, "%s:%d [%s]: " format "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

#endif

#define _LITTLE_ENDIAN_



using namespace std;

CborInput::CborInput(uint8_t *data, int size) : Bytes(data,size)
{
}

CborInput::~CborInput()
{

}

bool CborInput::hasBytes(int count)
{
    return available() >= count;
}

unsigned char CborInput::getByte()
{
    return read();
}



unsigned short CborInput::getShort()
{
    uint16_t value=0;
    unsigned int i;
    for(i=0; i<sizeof(value);i++)
    {
        value <<= 8;
        value += read();
    }
    return value;
}

unsigned int CborInput::getInt()
{
    uint32_t value=0;
    unsigned int i;
    for(i=0; i<sizeof(value);i++)
    {
        value <<= 8;
        value += read();
    }
    return value;
}

unsigned long long CborInput::getLong()
{
    uint64_t value=0;
    unsigned int i;
    for(i=0; i<sizeof(value);i++)
    {
        value <<= 8;
        value += read();
    }
    return value;
}

void CborInput::getBytes(uint8_t *to, int count)
{
     int i;
    for(i=0; i<count;i++)
    {
        *(to++) = read();
    }
}


CborReader::CborReader(CborInput &input)
{
    this->input = &input;
    this->state = STATE_TYPE;
}

CborReader::CborReader(CborInput &input, CborListener &listener)
{
    this->input = &input;
    this->listener = &listener;
    this->state = STATE_TYPE;
}

CborReader::~CborReader()
{

}

void CborReader::SetListener(CborListener &listener)
{
    this->listener = &listener;
}

void CborReader::Run()
{
    while(1)
    {
        if(state == STATE_TYPE)
        {
            if(input->hasBytes(1))
            {
                unsigned char type = input->getByte();
                unsigned char majorType = type >> 5;
                unsigned char minorType = type & 31;

                switch(majorType)
                {
                case 0: // positive integer
                    if(minorType < 24)
                    {
                        listener->OnInteger(minorType);
                    }
                    else if(minorType == 24)     // 1 byte
                    {
                        currentLength = 1;
                        state = STATE_PINT;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_PINT;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_PINT;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_PINT;
                    }
                    else
                    {
                        listener->OnError("invalid integer type");
                    }
                    break;
                case 1: // negative integer
                    if(minorType < 24)
                    {
                        listener->OnInteger(-minorType);
                    }
                    else if(minorType == 24)     // 1 byte
                    {
                        currentLength = 1;
                        state = STATE_NINT;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_NINT;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_NINT;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_NINT;
                    }
                    else
                    {
                        listener->OnError("invalid integer type");
                    }
                    break;
                case 2: // bytes
                    if(minorType < 24)
                    {
                        state = STATE_BYTES_DATA;
                        currentLength = minorType;
                    }
                    else if(minorType == 24)
                    {
                        state = STATE_BYTES_SIZE;
                        currentLength = 1;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_BYTES_SIZE;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_BYTES_SIZE;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_BYTES_SIZE;
                    }
                    else
                    {
                        listener->OnError("invalid bytes type");
                    }
                    break;
                case 3: // string
                    if(minorType < 24)
                    {
                        state = STATE_STRING_DATA;
                        currentLength = minorType;
                    }
                    else if(minorType == 24)
                    {
                        state = STATE_STRING_SIZE;
                        currentLength = 1;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_STRING_SIZE;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_STRING_SIZE;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_STRING_SIZE;
                    }
                    else
                    {
                        listener->OnError("invalid string type");
                    }
                    break;
                case 4: // array
                    if(minorType < 24)
                    {
                        listener->OnArray(minorType);
                    }
                    else if(minorType == 24)
                    {
                        state = STATE_ARRAY;
                        currentLength = 1;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_ARRAY;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_ARRAY;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_ARRAY;
                    }
                    else
                    {
                        listener->OnError("invalid array type");
                    }
                    break;
                case 5: // map
                    if(minorType < 24)
                    {
                        listener->OnMap(minorType);
                    }
                    else if(minorType == 24)
                    {
                        state = STATE_MAP;
                        currentLength = 1;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_MAP;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_MAP;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_MAP;
                    }
                    else
                    {
                        listener->OnError("invalid array type");
                    }
                    break;
                case 6: // tag
                    if(minorType < 24)
                    {
                        listener->OnTag(minorType);
                    }
                    else if(minorType == 24)
                    {
                        state = STATE_TAG;
                        currentLength = 1;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_TAG;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_TAG;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_TAG;
                    }
                    else
                    {
                        listener->OnError("invalid tag type");
                    }
                    break;
                case 7: // special
                    if(minorType < 24)
                    {
                        listener->OnSpecial(minorType);
                    }
                    else if(minorType == 24)
                    {
                        state = STATE_SPECIAL;
                        currentLength = 1;
                    }
                    else if(minorType == 25)     // 2 byte
                    {
                        currentLength = 2;
                        state = STATE_SPECIAL;
                    }
                    else if(minorType == 26)     // 4 byte
                    {
                        currentLength = 4;
                        state = STATE_SPECIAL;
                    }
                    else if(minorType == 27)     // 8 byte
                    {
                        currentLength = 8;
                        state = STATE_SPECIAL;
                    }
                    else
                    {
                        listener->OnError("invalid special type");
                    }
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_PINT)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    listener->OnInteger(input->getByte());
                    state = STATE_TYPE;
                    break;
                case 2:
                    listener->OnInteger(input->getShort());
                    state = STATE_TYPE;
                    break;
                case 4:
                    // TODO: for integers > 2^31
                    listener->OnInteger(input->getInt());
                    state = STATE_TYPE;
                    break;
                case 8:
                    input->getLong();
                    state = STATE_TYPE;
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_NINT)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    listener->OnInteger(-(int)input->getByte());
                    state = STATE_TYPE;
                    break;
                case 2:
                    listener->OnInteger(-(int)input->getShort());
                    state = STATE_TYPE;
                    break;
                case 4:
                    // TODO: for integers > 2^31
                    listener->OnInteger(-(int)input->getInt());
                    state = STATE_TYPE;
                    break;
                case 8:
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_BYTES_SIZE)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    currentLength = input->getByte();
                    state = STATE_BYTES_DATA;
                    break;
                case 2:
                    currentLength = input->getShort();
                    state = STATE_BYTES_DATA;
                    break;
                case 4:
                    currentLength = input->getInt();
                    state = STATE_BYTES_DATA;
                    break;
                case 8:
                    input->getLong();
                    currentLength = 123;
                    state = STATE_BYTES_DATA;
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_BYTES_DATA)
        {
            if(input->hasBytes(currentLength))
            {
                unsigned char *data = new unsigned char[currentLength];
                input->getBytes(data, currentLength);
                state = STATE_TYPE;
                listener->OnBytes(data, currentLength);
            }
            else break;
        }
        else if(state == STATE_STRING_SIZE)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    currentLength = input->getByte();
                    state = STATE_STRING_DATA;
                    break;
                case 2:
                    currentLength = input->getShort();
                    state = STATE_STRING_DATA;
                    break;
                case 4:
                    currentLength = input->getInt();
                    state = STATE_STRING_DATA;
                    break;
                case 8:
                    input->getLong();
                    currentLength = 123;
                    state = STATE_STRING_DATA;
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_STRING_DATA)
        {
            if(input->hasBytes(currentLength))
            {
                unsigned char *data = new unsigned char[currentLength];
                input->getBytes(data, currentLength);
                state = STATE_TYPE;
                Str str((uint8_t*)data,currentLength);
                listener->OnString(str);
//TODO				string str((const char *)data, (size_t)currentLength);
//TODO				listener->OnString(str);
            }
            else break;
        }
        else if(state == STATE_ARRAY)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    listener->OnArray(input->getByte());
                    state = STATE_TYPE;
                    break;
                case 2:
                    listener->OnArray(currentLength = input->getShort());
                    state = STATE_TYPE;
                    break;
                case 4:
                    listener->OnArray(input->getInt());
                    state = STATE_TYPE;
                    break;
                case 8:
                    //input->getLong();
                    //currentLength = 123;
                    state = STATE_TYPE;
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_MAP)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    listener->OnMap(input->getByte());
                    state = STATE_TYPE;
                    break;
                case 2:
                    listener->OnMap(currentLength = input->getShort());
                    state = STATE_TYPE;
                    break;
                case 4:
                    listener->OnMap(input->getInt());
                    state = STATE_TYPE;
                    break;
                case 8:
                    //input->getLong();
                    //currentLength = 123;
                    state = STATE_TYPE;
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_TAG)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    listener->OnTag(input->getByte());
                    state = STATE_TYPE;
                    break;
                case 2:
                    listener->OnTag(input->getShort());
                    state = STATE_TYPE;
                    break;
                case 4:
                    listener->OnTag(input->getInt());
                    state = STATE_TYPE;
                    break;
                case 8:
                    input->getLong();
                    state = STATE_TYPE;
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else if(state == STATE_SPECIAL)
        {
            if(input->hasBytes(currentLength))
            {
                switch(currentLength)
                {
                case 1:
                    listener->OnSpecial(input->getByte());
                    state = STATE_TYPE;
                    break;
                case 2:
                    listener->OnSpecial(input->getShort());
                    state = STATE_TYPE;
                    break;
                case 4:
                    // TODO: for integers > 2^31
                    listener->OnSpecial(input->getInt());
                    state = STATE_TYPE;
                    break;
                case 8:
                    input->getLong();
                    state = STATE_TYPE;
                    // TODO: implement it
                    break;
                }
            }
            else break;
        }
        else
        {
            logger("UNKNOWN STATE");
        }
    }
}

CborOutput::CborOutput(int capacity) : Bytes(capacity)
{
}

CborOutput::~CborOutput()
{

}

void CborOutput::putByte(unsigned char value)
{
    write(value);
}

void CborOutput::putBytes(const unsigned char *data, int size)
{
    int i;
    for(i=0;i<size;i++)
        write(data[i]);
}

CborWriter::CborWriter(CborOutput &output)
{
    this->output = &output;
}

CborWriter::~CborWriter()
{

}

unsigned char *CborOutput::getData()
{
    return data();
}

int CborOutput::getSize()
{
    return length();
}

void CborWriter::writeTypeAndValue(int majorType, unsigned int value)
{
    majorType <<= 5;
    if(value < 24)
    {
        output->putByte(majorType | value);
    }
    else if(value < 256)
    {
        output->putByte(majorType | 24);
        output->putByte(value);
    }
    else if(value < 65536)
    {
        output->putByte(majorType | 25);
        output->putByte(value >> 8);
        output->putByte(value);
    }
    else
    {
        output->putByte(majorType | 26);
        output->putByte(value >> 24);
        output->putByte(value >> 16);
        output->putByte(value >> 8);
        output->putByte(value);
    }
}

void CborWriter::writeTypeAndValue(int majorType, unsigned long long value)
{
    majorType <<= 5;
    if(value < 24ULL)
    {
        output->putByte(majorType | value);
    }
    else if(value < 256ULL)
    {
        output->putByte(majorType | 24);
        output->putByte(value);
    }
    else if(value < 65536ULL)
    {
        output->putByte(majorType | 25);
        output->putByte(value >> 8);
    }
    else if(value < 4294967296ULL)
    {
        output->putByte(majorType | 26);
        output->putByte(value >> 24);
        output->putByte(value >> 16);
        output->putByte(value >> 8);
        output->putByte(value);
    }
    else
    {
        output->putByte(majorType | 27);
        output->putByte(value >> 56);
        output->putByte(value >> 48);
        output->putByte(value >> 40);
        output->putByte(value >> 32);
        output->putByte(value >> 24);
        output->putByte(value >> 16);
        output->putByte(value >> 8);
        output->putByte(value);
    }
}

void CborWriter::writeInt(unsigned int value)
{
    writeTypeAndValue(0, value);
}

void CborWriter::writeInt(unsigned long long value)
{
    writeTypeAndValue(0, value);
}

void CborWriter::writeInt(long long value)
{
    if(value < 0)
    {
        writeTypeAndValue(1, (unsigned long long) -value);
    }
    else
    {
        writeTypeAndValue(0, (unsigned long long) value);
    }
}

void CborWriter::writeInt(int value)
{
    if(value < 0)
    {
        writeTypeAndValue(1, (unsigned int) -value);
    }
    else
    {
        writeTypeAndValue(0, (unsigned int) value);
    }
}

void CborWriter::writeBytes(const unsigned char *data, unsigned int size)
{
    writeTypeAndValue(2, size);
    output->putBytes(data, size);
}

void CborWriter::writeString(const char *data, unsigned int size)
{
    writeTypeAndValue(3, size);
    output->putBytes((const unsigned char *)data, size);
}

void CborWriter::writeString(const char *data)
{
    unsigned int size=strlen(data);
    writeTypeAndValue(3, size);
    output->putBytes((const unsigned char *)data, size);
}

void CborWriter::writeString(const Str& str)
{
    writeTypeAndValue(3, (unsigned int)str.length());
    output->putBytes((const unsigned char *)str.data(), str.length());
}


void CborWriter::writeArray(int size)
{
    writeTypeAndValue(4, (unsigned int)size);
}

void CborWriter::writeMap(int size)
{
    writeTypeAndValue(5, (unsigned int)size);
}

void CborWriter::writeTag(const unsigned int tag)
{
    writeTypeAndValue(6, tag);
}

void CborWriter::writeSpecial(int special)
{
    writeTypeAndValue(7, (unsigned int)special);
}

// TEST HANDLERS

void CborDebugListener::OnInteger(int value)
{
    printf("integer: %d\n", value);
}

void CborDebugListener::OnBytes(unsigned char *data, int size)
{
    printf("bytes with size: %d", size);
}

void CborDebugListener::OnString(Str &str)
{
    printf("string: '%.*s'\n", (int)str.length(), str.data());
}

void CborDebugListener::OnArray(int size)
{
    printf("array: %d\n", size);
}

void CborDebugListener::OnMap(int size)
{
    printf("map: %d\n", size);
}

void CborDebugListener::OnTag(unsigned int tag)
{
    printf("tag: %d\n", tag);
}

void CborDebugListener::OnSpecial(int code)
{
    printf("special: %d\n", code);
}

void CborDebugListener::OnError(const char *error)
{
    printf("error: %s\n", error);
}

// TEST HANDLERTE

void writeTest()
{
    CborOutput output(4096);
    CborWriter writer(output);

    writer.writeArray(3);
    writer.writeInt(123);
    writer.writeInt(12);
    writer.writeString("hello", 5);

    //fwrite(output.getData(), 1, output.getSize(), stdout);

    unsigned char *data = output.getData();
    int size = output.getSize();

    CborInput input(data, size);
    CborReader reader(input);
    reader.Run();
}

/*
int main(int argc, char **argv) {
	writeTest();
	return 0;
}
*/
