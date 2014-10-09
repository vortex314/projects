#include <iostream>
#include "Cbor.h"
#include "CborIn.h"
#include "CborOut.h"
#include <stdio.h>

using namespace std;

const char* names[]={"C_PINT","C_NINT","C_BYTES","C_STRING","C_ARRAY","C_MAP","C_TAG","C_SPECIAL","C_BOOL","C_FLOAT","C_DOUBLE","C_BREAK","C_NILL","C_ERROR"      } ;// minor additional types

class CborExampleListener : public CborInListener
{
public:
    Erc onToken(CborToken& t)  {
        cout << " type = " << names[t.type] << ", value = " ;
        switch ( t.type ) {
            case C_PINT : {
                cout << t.u._uint64;
                break;
            }
            case C_NINT : {
                cout << t.u._int64;
                break;
            }
            case C_STRING : {
                cout << t.value;
                break;
            }
            case C_BYTES : {
                cout << t.value;
                break;
            }
            case C_MAP : {
                cout << t.u._uint64;
                break;
            }
            case C_ARRAY : {
                cout << t.u._uint64;
                break;
            }
            case C_FLOAT : {
                cout << t.u._float;
                break;
            }
            case C_DOUBLE : {
                cout << t.u._double;
                break;
            }
            case C_BOOL : {
                cout << t.u._bool;
                break;
            }
            case C_TAG : {
                cout << t.u._uint64;
                break;
            }
            case C_BREAK : {
                cout << "BREAK";
                break;
            }
            case C_NILL : {
                cout << "NILL";
                break;
            }
            case C_ERROR : {
                cout << "ERROR";
                break;
            }
            case C_SPECIAL :{
                cout << t.u._uint64;
            }

        }
        cout << endl;
        return E_OK;
    }

};



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
    Str str(1000);

    cbo.add(1)
    .addArray(-1).add("Lieven").add("Merckx").addBreak()
    .add(-1)
    .add(true)
    .add(false)
    .add((float)1.23)   // float
    .add(1.23456)       //double
    .add("Hello world");
    cbo.addMap(2).add(1).add("een").add(2).add("twee");
    cbo.addArray(3).add("drie").add(4.0).add((float)5.0);
    cbo.addTag(1).add(-1);

    CborIn cbi(cbo.data(),cbo.length());
//  CborToken token;
    str.clear();

    cbi.toString(str);

    CborExampleListener cborExampleListener;

    cout << StrToString(str) << endl;

    cbi.offset(0);

    cbi.parse(cborExampleListener);


    return 0;
}
