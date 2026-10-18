#ifndef ME_MALLOC_H
#define ME_MALLOC_H

#if __has_include("mylibConf.h")
#include "mylibConf.h"

#include <stddef.h>
#include <stdint.h>

#ifndef ME_HEAP_SIZE
    #define ME_HEAP_SIZE (4096U)
#endif
#define ME_ALIGN        (4U)

/* ---------------------------------------------------------------
 * API interne (ne pas appeler directement)
 * --------------------------------------------------------------- */
void     meHeapInit();
void    *_meMalloc(uint16_t size, const char *file, int line);
void     _meFree(void *ptr);
uint16_t meHeapFree();
uint16_t meHeapUsed();
void meHeapDump();

/* ---------------------------------------------------------------
 * Macro publique — capture automatiquement fichier + ligne
 * Usage :  uint8_t *buf = meMalloc(64);
 * --------------------------------------------------------------- */
#define meMalloc(_size)  _meMalloc((_size), __FILE_NAME__, __LINE__)
#define meFree(_ptr)     _meFree(_ptr)

#endif // __has_include

#endif // ME_MALLOC_H