#ifndef __GENLS_H
#define __GENLS_H

#include <stdbool.h>
#include "stm32f0xx.h"
#include "meFifo.h"
#include "meFd.h"

#define DECLARE_LS_FN(_name)                                    \
                                                                \
void USART_##_name##_Putc(void *ctx, char c) {                  \
    meCtx_usart_t *p = (meCtx_usart_t*)ctx;                     \
                                                                \
    p->usart->CR1 &= ~USART_CR1_TXEIE;                          \
                                                                \
    if (*p->isTransmitting)                                     \
    {                                                           \
        meFifo_Putc(p->fifoTx, c);                              \
    }else                                                       \
    {                                                           \
        p->usart->TDR = c;                                      \
        *p->isTransmitting = true;                              \
    }                                                           \
                                                                \
    p->usart->CR1 |= USART_CR1_TXEIE;                           \
}                                                               \
                                                                \
bool USART_ ## _name##_Getc(void *ctx, char *c) {               \
    meCtx_usart_t *p = (meCtx_usart_t*)ctx;                     \
    return meFifo_Readc(p->fifoRx, c);                          \
}                                                               \
                                                                \
bool USART_ ## _name##_CanWrite(void *ctx, uint16_t n) {        \
    meCtx_usart_t *p = (meCtx_usart_t*)ctx;                     \
    return meFifoFree(p->fifoRx, n);                            \
}                                                               \
                                                                \
bool USART_ ## _name##_CanRead(void *ctx, uint16_t n) {         \
    meCtx_usart_t *p = (meCtx_usart_t*)ctx;                     \
    return meFifoCnt(p->fifoRx, n);                             \
}



#define _USART_DECLARE(_name, _usartNb, _irqHandler, _irqn, _rccFn, _rccPeriph,    \
                       _portRX, _pinRX, _portTX, _pinTX, _AF, _sizeRX, _sizeTX, _baud) \
                                                                                    \
    ME_FIFO_INIT(fifoRx_##_name, _sizeRX);                                          \
    ME_FIFO_INIT(fifoTx_##_name, _sizeTX);                                          \
    bool isTransmitting_##_name = false;                                            \
    static meCtx_usart_t meCtx_##_name;                                             \
                                                                                    \
    void _irqHandler(void)                                                          \
    {                                                                               \
        if (USART##_usartNb->ISR & USART_ISR_RXNE) {                                \
            char c = (uint8_t)USART##_usartNb->RDR;                                 \
            meFifo_Putc(&fifoRx_##_name, c);                                        \
        }                                                                           \
        if (USART##_usartNb->ISR & USART_ISR_TXE) {                                 \
            if (!meFifoIsEmpty(&fifoTx_##_name)) {                                  \
                char c;                                                             \
                meFifo_Readc(&fifoTx_##_name, &c);                                  \
                USART##_usartNb->TDR = c;                                           \
            } else {                                                                \
                isTransmitting_##_name = false;                                     \
                USART##_usartNb->CR1 &= ~USART_CR1_TXEIE;                           \
            }                                                                       \
        }                                                                           \
    }                                                                               \
                                                                                    \
    void usart_init_##_name(void)                                                   \
    {                                                                               \
        _rccFn(_rccPeriph, ENABLE);                                                 \
                                                                                    \
        GPIO_InitTypeDef gpio = {0};                                                \
        gpio.GPIO_Mode  = GPIO_Mode_AF;                                             \
        gpio.GPIO_OType = GPIO_OType_PP;                                            \
        gpio.GPIO_Speed = GPIO_Speed_50MHz;                                         \
        gpio.GPIO_PuPd  = GPIO_PuPd_UP;                                             \
        gpio.GPIO_Pin   = 1u << (_pinRX);                                           \
        GPIO_Init(_portRX, &gpio);                                                  \
        gpio.GPIO_Pin   = 1u << (_pinTX);                                           \
        GPIO_Init(_portTX, &gpio);                                                  \
        GPIO_PinAFConfig(_portRX, _pinRX, _AF);                                     \
        GPIO_PinAFConfig(_portTX, _pinTX, _AF);                                     \
                                                                                    \
        USART_InitTypeDef u = {0};                                                  \
        u.USART_BaudRate   = _baud;                                                 \
        u.USART_WordLength = USART_WordLength_8b;                                   \
        u.USART_StopBits   = USART_StopBits_1;                                      \
        u.USART_Parity     = USART_Parity_No;                                       \
        u.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;                         \
        USART_Init(USART##_usartNb, &u);                                            \
                                                                                    \
        USART##_usartNb->CR1 |= USART_CR1_RXNEIE;                                   \
        NVIC_SetPriority(_irqn, 0);                                                 \
        NVIC_EnableIRQ(_irqn);                                                      \
        USART_Cmd(USART##_usartNb, ENABLE);                                         \
                                                                                    \
        meCtx_##_name.fifoRx         = &fifoRx_##_name;                             \
        meCtx_##_name.fifoTx         = &fifoTx_##_name;                             \
        meCtx_##_name.usart          = USART##_usartNb;                             \
        meCtx_##_name.isTransmitting = &isTransmitting_##_name;                     \
    }                                                                               \
                                                                                    \
    DECLARE_LS_FN(_name)                                                            \
                                                                                    \
    void meFd_init_##_name(meFd_t *fd)                                              \
    {                                                                               \
        static const meOps_t ops = {                                                \
            .putc     = USART_ ## _name##_Putc,                                     \
            .getc     = USART_ ## _name##_Getc,                                     \
            .canWrite = USART_ ## _name##_CanWrite,                                 \
            .canRead  = USART_ ## _name##_CanRead,                                  \
        };                                                                          \
        fd->ops = (meOps_t*)&ops;                                                   \
        fd->ctx = &meCtx_##_name;                                                   \
    }



#define USART2_DECLARE_PA2_PA3(_name, _baud, _sizeRX, _sizeTX) _USART_DECLARE( _name, 2, USART2_IRQHandler, USART2_IRQn, RCC_APB1PeriphClockCmd, RCC_APB1Periph_USART2, \
                                                                               GPIOA, 3, GPIOA, 2, GPIO_AF_1, _sizeRX, _sizeTX, _baud)

#endif // __GENLS_H