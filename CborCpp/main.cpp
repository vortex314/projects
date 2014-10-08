#include <iostream>
#include "Cbor.h"
#include "CborIn.h"
#include "CborOut.h"
#include <stdio.h>

using namespace std;

class CborExampleListener : public CborListener
{
public:
    virtual void OnInteger(int value);
    virtual void OnBytes(unsigned char *data, int size);
    virtual void OnString(Str &str);
    virtual void OnArray(int size);
    virtual void OnMap(int size);
    virtual void OnTag(unsigned int tag);
    virtual void OnSpecial(int code);
    virtual void OnError(const char *error);
};


void CborExampleListener::OnInteger(int value)
{
    printf("integer: %d\n", value);
}

void CborExampleListener::OnBytes(unsigned char *data, int size)
{
    printf("bytes with size: %d", size);
}

void CborExampleListener::OnString(Str &str)
{
    printf("string: '%.*s'\n", (int)str.length(), str.data());
}

void CborExampleListener::OnArray(int size)
{
    printf("array: %d\n", size);
}

void CborExampleListener::OnMap(int size)
{
    printf("map: %d\n", size);
}

void CborExampleListener::OnTag(unsigned int tag)
{
    printf("tag: %d\n", tag);
}

void CborExampleListener::OnSpecial(int code)
{
    printf("special: %d\n", code);
}

void CborExampleListener::OnError(const char *error)
{
    printf("error: %s\n", error);
}


std::string gStr;

string StrToString(Str& str)
{
    str.offset(0);
    gStr.clear();
    while(str.hasData())
        gStr+=str.read();
    return gStr;
}


int main()
{
    CborOut cbo(100);
        Str str(100);

    cbo.add(true)
    .add(false)
    .add((float)1.23)
    .add("Hello world");

    CborIn cbi(cbo.data(),cbo.length());
//  CborToken token;
    str.clear();

    cbi.toString(str);

    cout << StrToString(str) << endl;



    CborOutput output(9000);
    CborWriter writer(output);

    uint8_t bytes[]= {1,2,0xFF,0xFE};

    writer.write((float)1.23);
    writer.write(true);
    writer.write(false);

    writer.writeString("hello");
    writer.writeString("world");
    writer.writeBytes(bytes,sizeof(bytes));
    writer.writeMap(1);
    writer.writeInt(1);
    writer.writeString("Een");
    writer.writeInt(321);
    writer.writeInt(-321);

    writer.writeTag(123);
    writer.writeArray(2);
    writer.writeString("hello");
    writer.writeMap(1);
    writer.writeString("world");
    writer.writeInt(321);

    writer.writeInt(12235666);



    output.toString(str);

    cout << StrToString(str) << endl;



    unsigned char *data = output.getData();
    int size = output.getSize();

    CborInput input(data, size);
    CborReader reader(input);
    CborExampleListener listener;
    reader.SetListener(listener);
    reader.Run();

    CborIn cbi2(data,size);
//  CborToken token;
    str.clear();

    cbi2.toString(str);

    cout << StrToString(str) << endl;


    cout << "Hello world!" << endl;
    return 0;
}
