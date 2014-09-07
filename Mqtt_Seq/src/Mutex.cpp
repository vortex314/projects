#include "Mutex.h"

Mutex::Mutex()
{
    //ctor
}

Mutex::~Mutex()
{
    //dtor
}
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

pthread_mutex_t lck;


void Mutex::lock(){
    pthread_mutex_lock(&lck);
}


void Mutex::unlock(){
    pthread_mutex_unlock(&lck);
}
