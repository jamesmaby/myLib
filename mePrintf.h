#ifndef __MEPRINTF_H
#define __MEPRINTF_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    bool minus_sign;   // Si je recois un signe - ex : "james %-10"
    uint8_t len;       // Longueur de la chaine de caractère à envoyer
    uint8_t space;      // Ecart entre le nombre de caractère du format (%10) et la longueur de la chaine
    uint8_t nbCarFmt;    
}format_t;

typedef struct {
    char * buff;
    uint8_t index;
    uint16_t size;
}sprintfPutc_t;

typedef void (*fnPutc_t)(void *, char c);

void tinyFormat(fnPutc_t putc, void *ctx, const char *format, va_list va);

uint32_t meAtoi(const char * s);
uint8_t meAtoiChar(const char c);
void meIntToBinary(uint8_t num, char *buffer);
void meIntToHex(uint32_t num, char *buffer);
void meItoa(uint32_t num,char * buff);

#endif  //__MEPRINTF_H
