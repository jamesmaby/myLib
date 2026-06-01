#include "meFd.h"

uint16_t meFd_Write(meFd_t *p, char *buff, uint16_t n){
    uint16_t i = 0;
    if (p->ops->canWrite(p->ctx, n))
    {
        while (*buff){
            p->ops->putc(p->ctx, *buff++);
            i++;
        }
    }
    return i;
}

uint16_t meFd_Read(meFd_t *p, char *buff, uint16_t n){
    uint16_t i = 0;
    if (p->ops->canRead(p->ctx, n))
    {
        while (i < n){
            p->ops->getc(p->ctx, buff);
            buff++;
            i++;
        }
    }
    return i;
}

uint16_t meFd_Print(meFd_t *p, char *buff){
    uint16_t i = 0;

    while (*buff){
        p->ops->putc(p->ctx, *buff);
        buff++;
        i++;
    }
    return i;
}


void meFd_Printf(meFd_t *me_sd, const char *format, ...) {
    va_list va;

    va_start(va, format);

    tinyFormat(me_sd->ops->putc, me_sd->ctx, format, va);
    
    va_end(va);
}

/*  */
static void sputc(void *contexte, char c){
    sprintf_putc_t *ctx = (sprintf_putc_t*)contexte;

    if (ctx->index < ctx->size - 1) {
        ctx->buffer[ctx->index++] = c;
    }
}

void meFd_Sprintf(char *buf, uint8_t size, const char *format, ...){

    sprintf_putc_t ctx = { .buffer = buf, .index = 0, .size = size };
    va_list va;
    va_start(va, format);

    tinyFormat(sputc, &ctx, format, va);

    va_end(va);

    ctx.buffer[ctx.index] = '\0';

}
