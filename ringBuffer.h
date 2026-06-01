#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include <stdint.h>
#include <stdbool.h>

struct CONCTX_T;

char* rb_prev(const char* pb, uint16_t sz, char *p);
char* rb_next(const char* pb, uint16_t sz, char *p);
bool rb_strcmp(struct CONCTX_T *ctx, char *p, const char *s);
void rb_to_string(struct CONCTX_T *p, char* buf, uint16_t size);
uint16_t rb_lenBuff(struct CONCTX_T *ctx);


#endif // __RINGBUFFER_H