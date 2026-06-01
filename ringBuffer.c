#include "ringBuffer.h"
#include "meConsole.h"


inline char* rb_next(const char* base, uint16_t size, char *p) {
    return ((p+1-base) >= (size)) ? (char*)base : p+1;
}

inline char* rb_prev(const char* base, uint16_t size, char *p) {
    return ((p <= base)) ? (char*)(base+size-1) : p-1;
}

uint16_t rb_lenBuff(struct CONCTX_T *ctx){
    uint8_t len = 0;
    char* pb = ctx->pBuf_current;
    while (*pb){
        len++;
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }
    return len;
}

void rb_to_string(struct CONCTX_T *p, char* buf, uint16_t size) {
    char *pb   = buf;
    char *pcur = p->pBuf_current;

    while (*pcur && (pb - buf < size - 1)) {
        *pb++ = *pcur;
        pcur = rb_next(p->buffer, CONS_BUFFER_SIZE, pcur);
    }
    *pb = '\0';
}

bool rb_strcmp(struct CONCTX_T *ctx, char *p, const char *s) {
    while (*s)
    {
        if (*p != *s)
            return false;

        p = rb_next(ctx->buffer, CONS_BUFFER_SIZE, p);
        s++;
    }

    return (*p == ' ' || *p == '\0');
}