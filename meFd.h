#ifndef __ME_FD_H
#define __ME_FD_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "stm32f0xx.h"
#include "meFifo.h"
#include "mePrintf.h"

typedef struct
{
    void (*putc)(void *ctx, char c);
    bool (*getc)(void *ctx, char *c);
    bool (*canWrite)(void *ctx, uint16_t n);
    bool (*canRead)(void *ctx, uint16_t n);
}meOps_t;

typedef struct
{
    meFifo_t *fifoRx;
    meFifo_t *fifoTx;

    USART_TypeDef *usart;
    bool* isTransmitting;
}meCtx_usart_t;


typedef struct
{
    meOps_t *ops;
    meCtx_usart_t *ctx;
}meFd_t;

typedef struct 
{
    char * buffer;
    uint8_t index;
    uint16_t size;
}sprintf_putc_t;

// void meFd_Init(meFd_t *p);
uint16_t meFd_Write(meFd_t *p, char *buff, uint16_t n);
uint16_t meFd_Read(meFd_t *p, char *buff, uint16_t n);
uint16_t meFd_Print(meFd_t *p, char *buff);
void meFd_Printf(meFd_t *me_sd, const char *format, ...);
void meFd_Sprintf(char *buf, uint8_t size, const char *format, ...);


#endif // __ME_FD_H
