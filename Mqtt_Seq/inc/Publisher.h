#ifndef PUBLISHER_H_INCLUDED
#define PUBLISHER_H_INCLUDED
#include <stddef.h> // NULL
template <class TL>
class Publisher
{
private :
    typedef struct ListenerStruct
    {
        struct ListenerStruct *next;
        TL* listener;
    } ListenerNode;
public:
    void addListener(TL* listener)
    {
        ListenerNode* cursor;
        if ( _first == NULL )
        {
            _first = new ListenerNode();
            cursor=_first;
        }
        else
        {
            cursor=_first;
            while(cursor->next != NULL ) cursor=cursor->next;
            cursor->next = new ListenerNode();
            cursor=cursor->next;
        }
        cursor->listener=listener;
    }
    ListenerNode* firstListener()
    {
        return _first;
    }


private:
    ListenerNode* _first;
};



#endif // PUBLISHER_H_INCLUDED
