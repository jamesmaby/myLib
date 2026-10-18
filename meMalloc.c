#include "meMalloc.h"
#include <string.h>
#include "meConsole.h"

#define ALIGN_UP(x)     (((x) + (ME_ALIGN - 1U)) & ~(ME_ALIGN - 1U))
#define MIN_BLOCK_SIZE  (ALIGN_UP(sizeof(BlockHeader)) + ME_ALIGN)

typedef struct BlockHeader {
    uint16_t            size;
    uint8_t             free;
    const char         *file;   /* fichier source de l'allocation */
    uint16_t            line;   /* ligne source de l'allocation   */
    struct BlockHeader *next;
} BlockHeader;

static uint8_t      heap[ME_HEAP_SIZE] __attribute__((aligned(ME_ALIGN)));
static BlockHeader *heapStart = NULL;

/* ---------------------------------------------------------------
 * meHeapInit
 * --------------------------------------------------------------- */
void meHeapInit()
{
    heapStart       = (BlockHeader *)heap;
    heapStart->size = ME_HEAP_SIZE - ALIGN_UP(sizeof(BlockHeader));
    heapStart->free = 1;
    heapStart->file = NULL;
    heapStart->line = 0;
    heapStart->next = NULL;
}

/* ---------------------------------------------------------------
 * _meMalloc  (appelé via la macro meMalloc)
 * --------------------------------------------------------------- */
void *_meMalloc(uint16_t size, const char *file, int line)
{
    BlockHeader *current;
    BlockHeader *newBlock;
    uint16_t     alignedSize;

    if (size == 0U) return NULL;

    alignedSize = (uint16_t)ALIGN_UP(size);

    current = heapStart;
    while (current != NULL) {
        if (current->free && current->size >= alignedSize) {

            if (current->size >= alignedSize + MIN_BLOCK_SIZE) {
                newBlock       = (BlockHeader *)((uint8_t *)current
                                 + (uint16_t)ALIGN_UP(sizeof(BlockHeader))
                                 + alignedSize);
                newBlock->size = current->size
                                 - alignedSize
                                 - (uint16_t)ALIGN_UP(sizeof(BlockHeader));
                newBlock->free = 1;
                newBlock->file = NULL;
                newBlock->line = 0;
                newBlock->next = current->next;

                current->size  = alignedSize;
                current->next  = newBlock;
            }

            current->free = 0;
            current->file = file;
            current->line = (uint16_t)line;

            // printf("[meMalloc] +%u bytes @ %s:%d\n", alignedSize, file, line);

            return (void *)((uint8_t *)current + ALIGN_UP(sizeof(BlockHeader)));
        }
        current = current->next;
    }

    // printf("[meMalloc] FAILED %u bytes @ %s:%d\n", alignedSize, file, line);
    return NULL;
}

/* ---------------------------------------------------------------
 * meFree
 * --------------------------------------------------------------- */
void _meFree(void *ptr)
{
    BlockHeader *block;
    BlockHeader *current;
    uint8_t found = 0;

    if (ptr == NULL) return;

    block       = (BlockHeader *)((uint8_t *)ptr - ALIGN_UP(sizeof(BlockHeader)));
    block->free = 1;
    block->file = NULL;
    block->line = 0;

    /* Coalescing */
    current = heapStart;
    while (current != NULL) {
        if (current->free && current->next != NULL && current->next->free) {
            current->size += (uint16_t)ALIGN_UP(sizeof(BlockHeader))
                             + current->next->size;
            current->next  = current->next->next;
            
            if (!found) found = 1;

        } else {
            current = current->next;
        }
    }
    if (found) ptr = NULL;
}

/* ---------------------------------------------------------------
 * Diagnostics
 * --------------------------------------------------------------- */
uint16_t meHeapFree()
{
    BlockHeader *c = heapStart;
    uint16_t     n = 0;
    while (c) { if (c->free)  n += c->size; c = c->next; }
    return n;
}

uint16_t meHeapUsed()
{
    BlockHeader *c = heapStart;
    uint16_t     n = 0;
    while (c) { if (!c->free) n += c->size; c = c->next; }
    return n;
}

cons_cmd_rc_t console_fn_Mem(conCtx_t *pCtx);

COMMANDE(mem, "mem", console_fn_Mem, "Donne les infos d'allocations mémoires",
        "     mem:      Donne les infos d'allocations mémoires\r\n"
);

cons_cmd_rc_t console_fn_Mem(conCtx_t *pCtx){
    int status = CON_RC_DONE;

    if ( ARGNB(0) )
    {
        BlockHeader *c = heapStart;
    
        cprintf("=== Heap dump (used=%d free=%d) ===\r\n", meHeapUsed(), meHeapFree());
        while (c != NULL) {
            if (!c->free) {
                cprintf(" @%08X [%dB]    %-20s:%d\r\n",
                    c, c->size,
                    c->file ? c->file : "?",
                    c->line);
            }
            c = c->next;
        }
        cprintf("===================================\r\n");

    } else 
    {
        cprintf("bad usage : \r\n%s\r\n", pCtx->com_cmd_desc->usage);
        console_Prompt(pCtx);
        status = CON_RC_BAD_ARG;
    }

    return status;
}

