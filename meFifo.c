#include <string.h>

#include "meFifo.h"
#include "stm32f0xx.h"


bool meFifo_Putc(meFifo_t *p, char c){
    uint16_t nextHead = (p->head + 1) & p->msk;
    if ( nextHead != p->tail ){
        p->buffer[p->head] = c;
        p->head = nextHead;
        return 0;
    }else 
        return 1;
}

bool meFifo_Readc(meFifo_t *p, char *c){
    if ( meFifoCanRead(p) ){
        *c = p->buffer[p->tail];
        p->tail = (p->tail + 1) & p->msk;
        return 0;
    }else 
        return 1;
}

bool meFifoCanRead(meFifo_t *p){
    return !meFifoIsEmpty(p);
}

bool meFifoCanWrite(meFifo_t *p){
    return !meFifoIsFull(p);
}

bool meFifoIsEmpty(meFifo_t *p){
    return (p->head == p->tail);
}

bool meFifoIsFull(meFifo_t *p){
    return (p->head == ((p->tail+1) & p->msk));
}

bool meFifoCnt(meFifo_t *p, uint16_t n){
    uint16_t diff = p->head - p->tail;

    if (diff >= 0)
        n = diff;
    else
        n = p->msk - (-diff - 1);
        
    if (n)
        return (bool)n;

    return (bool)n;
}

bool meFifoFree(meFifo_t *p, uint16_t n){
    uint16_t diff = p->head - p->tail;

    if (diff >= 0)
        n = p->msk - diff;
    else
        n = -diff - 1;

    return (bool)n;
}

