#ifndef MECONSOLECARAC
#define MECONSOLECARAC

#include "meConsole.h"
#include <stdbool.h>
#include "mePrintf.h"




void console_Clear(conCtx_t *ctx);
bool console_entrer(conCtx_t *ctx);
void console_Backspace(conCtx_t *ctx);
void console_LeftArrow(conCtx_t *ctx);
void console_RightArrow(conCtx_t *ctx);
void console_DownArrow(conCtx_t *ctx);
void console_UpArrow(conCtx_t *ctx);
void console_Echap(conCtx_t *ctx);
void console_Carac (conCtx_t *ctx, char c);
void console_Home(conCtx_t *ctx);
void console_End(conCtx_t *ctx);
void console_Del(conCtx_t *ctx);
void console_Tabulation(conCtx_t *ctx);
void console_CtrlLeftArrow(conCtx_t *ctx);
void console_CtrlRightArrow(conCtx_t *ctx);

// void console_CtrlC(conCtx_t *ctx);



#endif