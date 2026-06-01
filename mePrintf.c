#include "mePrintf.h"

#include <stdarg.h>


void tinyFormat(fnPutc_t putc, void *ctx, const char *format, va_list va){
    format_t f;

    char binary[8];
    char hex[8];
    char d_buff[11];

    f.nbCarFmt = 0;
    f.minus_sign = false;

    while (*format) {
        if (*format != '%') {
          putc(ctx,*format);
        } else {
            format++; 
            f.nbCarFmt = 0;
            f.minus_sign = false;

            while (*format == '-' || (*format >= '0' && *format <= '9')){
                if (*format == '-') {
                    f.minus_sign = true;
                } else {
                    f.nbCarFmt = f.nbCarFmt * 10 + (*format - '0');  // Dans le cas ou le caractère suivant '%' est un chiffre
                }
                format++;
            }
            switch (*format) {
                case '%':
                    putc(ctx, *format);
                    break;
                    
                case 'c': {  
                    char v = (char) va_arg(va, int);
                    putc(ctx,v);
                    break;
                }
                case 's': {  
                    char *s = va_arg(va, char *);
                    f.len = strlen(s);
                    f.space = (f.nbCarFmt > f.len) ? (f.nbCarFmt - f.len) : 0;

                    if (!f.space){
                        while (*s){
                            putc(ctx,*(s++));
                        }
                        goto b;
                    }

                    if (f.minus_sign == false) {

                        while (*s && ( f.nbCarFmt-- > 0 )) {
                            putc(ctx,*(s++));
                        }
                        for (int i = 0; i < f.space; i++) {
                            putc(ctx,' ');
                        }
                    } else {
                        for (int i = 0; i < f.space; i++) {
                            putc(ctx,' ');
                        }
                        while (*s && ( f.nbCarFmt-- > 0 )) {
                            putc(ctx,*(s++));
                        }
                    }
                    b:
                    break;
                }
                case 'B' :
                case 'b' : {
                    uint32_t b = va_arg(va, int);
                    meIntToBinary((uint8_t) b, binary);
                    char* pb = binary;

                    int len = strlen(binary);
                    int pad = (f.nbCarFmt > len) ? (f.nbCarFmt - len) : 0;


                    if (*(format-2) == '0') {  // simple détection du '0' dans "%02d"
                        for (int i = 0; i < pad; i++) {
                            putc(ctx,'0');
                        }
                    } else if (pad > 0) {  // sinon espace
                        for (int i = 0; i < pad; i++) {
                            putc(ctx,' ');
                        }
                    }

                    while (*pb){
                        putc(ctx,*(pb++));
                    }
                    break;
                }
                case 'X' :
                case 'x' : {
                    uint32_t x = va_arg(va, int);
                    meIntToHex((uint32_t) x, hex);
                    char * phex = hex;

                    int len = strlen(hex);
                    int pad = (f.nbCarFmt > len) ? (f.nbCarFmt - len) : 0;

                    if (*(format-2) == '0') {  // simple détection du '0' dans "%02d"
                        for (int i = 0; i < pad; i++) {
                            putc(ctx,'0');
                        }
                    } else if (pad > 0) {  // sinon espace
                        for (int i = 0; i < pad; i++) {
                            putc(ctx,' ');
                        }
                    }

                    while (*phex){
                        putc(ctx,*(phex++));
                    }
                    break;
                }
                case 'd' : {
                    uint64_t d = va_arg(va, int);
                    meItoa((uint64_t) d,d_buff);
                    char * pd = d_buff;

                    int len = strlen(d_buff);
                    int pad = (f.nbCarFmt > len) ? (f.nbCarFmt - len) : 0;

                    if (*(format-2) == '0') {  // simple détection du '0' dans "%02d"
                        for (int i = 0; i < pad; i++) {
                            putc(ctx,'0');
                        }
                    } else if (pad > 0) {  // sinon espace
                        for (int i = 0; i < pad; i++) {
                            putc(ctx,' ');
                        }
                    }
                    while (*pd){
                        putc(ctx,*(pd++));
                    }
                    break;
                }
                default:
                    putc(ctx,'?'); 
                    break;
            }
        }

        format++;
    }
    putc(ctx,'\0');
    va_end(va);
}

uint32_t meAtoi(const char * s){
    uint8_t buffer[10];
    uint8_t * ptr = buffer;

    uint32_t val = 0;
    uint32_t puiss = 1;
    uint8_t len = 0;

    while (*s && len < 10) {  
        *(ptr++) = *(s++) - '0';
        len++;
    }

    for (int i = len - 1; i >= 0; i--) {
        val += buffer[i] * puiss;
        puiss *= 10;
    }

    return val;
}


uint8_t meAtoiChar(const char c) {
    if (c >= '0' && c <= '9') {
        return c - '0'; 
    }
    return 0;
}

void meIntToBinary(uint8_t num, char *buffer) {
    uint8_t i, j = 0;
    uint8_t start = 0;  

    for (i = 0; i < 8; i++) {

        if ((num >> (7 - i)) & 1) {
            start = 1;  
        }

        if (start) {
            buffer[j++] = ((num >> (7 - i)) & 1) + '0';  
        }
    }

    if (j == 0) {  
        buffer[j++] = '0';  
    }
    buffer[j] = '\0';  
}

void meIntToHex(uint32_t num, char *buf){
    static const char *hex = "0123456789ABCDEF";
    char tmp[9]; // max 8 hex digits + '\0'
    int i = 0;

    if (num == 0) {
        buf[0] = '0';
        goto end;
    }

    while (num) 
    {
        tmp[i++] = hex[num & 0xF];
        num >>= 4;
    }

    // inverser dans buf
    for (int j = 0; j < i; j++) 
        buf[j] = tmp[i - j - 1];
    

end:
    buf[i] = '\0'; // fin de chaîne
}


void meItoa(uint32_t num,char * buff){

    uint8_t i = 0;
    
    if (num == 0) {  
        buff[i++] = '0';  
    } else {
        while (num > 0) {
            buff[i++] = (num % 10) + '0'; 
            num /= 10; 
        }
    }

    buff[i] = '\0';  

    for (uint8_t j = 0; j < i / 2; j++) {
        char temp = buff[j];
        buff[j] = buff[i - 1 - j];
        buff[i - 1 - j] = temp;
    }
}