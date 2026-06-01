#ifndef __ME_FIFO_H
#define __ME_FIFO_H

#include <stdint.h>
#include <stdbool.h>

#define ME_FIFO_INIT(name, size)\
    uint8_t name##_buffer[size] = {0};\
    meFifo_t name = {\
        .buffer = name##_buffer,\
        .head = 0,\
        .tail = 0,\
        .msk = size - 1\
    }

typedef struct
{
    uint8_t *buffer;
    uint16_t head;
    uint16_t tail;
    uint16_t msk;
}meFifo_t;



bool meFifo_Putc(meFifo_t *p, char c);
bool meFifo_Readc(meFifo_t *p, char *c);
bool meFifoCanRead(meFifo_t *p);
bool meFifoCanWrite(meFifo_t *p);
bool meFifoIsEmpty(meFifo_t *p);
bool meFifoIsFull(meFifo_t *p);
bool meFifoCnt(meFifo_t *p, uint16_t n);
bool meFifoFree(meFifo_t *p, uint16_t n);

#endif // __ME_FIFO_H
