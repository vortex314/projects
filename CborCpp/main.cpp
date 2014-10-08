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

    cbo.add(1)
    .add(-1)
    .add(true)
    .add(false)
    .add((float)1.23)   // float
    .add(1.23456)       //double
    .add("Hello world");
    cbo.addMap(2).add(1).add("een").add(2).add("twee");
    cbo.addArray(3).add("drie").add(3.0).add((float)4.0);

    CborIn cbi(cbo.data(),cbo.length());
//  CborToken token;
    str.clear();

    while ( cbi.hasData() )
    {

        cbi.toString(str);
        if ( cbi.hasData() ) str <<",";
    };

    cout << StrToString(str) << endl;


    return 0;
}
