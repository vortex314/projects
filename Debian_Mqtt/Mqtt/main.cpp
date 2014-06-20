#include <iostream>
#include "Bytes.h"
#include "Str.h"

using namespace std;



int main()
{
    cout << "Hello world!" << endl;
    Bytes bytes(10);
Str str(200);
    str.append("Hello world ");
    cout << str.data();
    return 0;
}
