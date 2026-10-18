#include "meConsole.h"
#include "meConsoleCarac.h"
#include "stm32f0xx.h"
#include "system.h"
#include "meMalloc.h"

#define ESC_TIMEOUT (50)

#define CON_CTX_ALLOC(_ctx) \
    (_ctx*)meMalloc(sizeof(_ctx))

#define VT100_STR_BLACK           "\033[30m"
#define VT100_STR_RED             "\033[31m"
#define VT100_STR_GREEN           "\033[32m"
#define VT100_STR_YELLOW          "\033[33m"
#define VT100_STR_BLUE            "\033[34m"
#define VT100_STR_MAGENTA         "\033[35m"
#define VT100_STR_CYAN            "\033[36m"
#define VT100_STR_WHITE           "\033[37m"

#define VT100_STR_BRIGHT_BLACK    "\033[90m"
#define VT100_STR_BRIGHT_RED      "\033[91m"
#define VT100_STR_BRIGHT_GREEN    "\033[92m"
#define VT100_STR_BRIGHT_YELLOW   "\033[93m"
#define VT100_STR_BRIGHT_BLUE     "\033[94m"
#define VT100_STR_BRIGHT_MAGENTA  "\033[95m"
#define VT100_STR_BRIGHT_CYAN     "\033[96m"
#define VT100_STR_BRIGHT_WHITE    "\033[97m"

#define VT100_STR_RESET           "\033[0m"

#define DEF_SEQ( _seq, _code) { _seq, sizeof(_seq) - 1, _code }
const cons_sp_char_t cons_sp_char[] = {
    DEF_SEQ( "\x1B[D",  LEFT_ARROW),
    DEF_SEQ( "\x1B[C",  RIGHT_ARROW),
    DEF_SEQ( "\x1B[A",  UP_ARROW),
    DEF_SEQ( "\x1B[B",  DOWN_ARROW),
    DEF_SEQ( "\x1B[4~", FIN),
    DEF_SEQ( "\x1B[1~", DEBUT),
    DEF_SEQ( "\x1B[3~", DEL),
    DEF_SEQ( "\x1BOD",  CTRL_LEFT_ARROW),
    DEF_SEQ( "\x1BOC",  CTRL_RIGHT_ARROW),
    DEF_SEQ( "\x1B",    ECHAP),
    DEF_SEQ( NULL,      NO_INPUT),
};

COMMANDE(help, "help", console_fn_Help, "Donne toutes les commandes possibles", "help [command]" );
COMMANDE(reboot, "reboot", console_fn_Reboot, "Reboot le système", "reboot" );
COMMANDE(test, "test", console_fn_Test, "Ceci est une fonction test", "test" );
COMMANDE(uptime, "uptime", console_fn_Uptime, "Donne le temps écoulé depuis le démarrage système", "uptime [-f]" );
COMMANDE(clear, "clear", console_fn_Clear, "Clear la console", "clear") ;
COMMANDE(color, "color", console_fn_Color, "Set print color", "color <col>") ;

void meFd_init_console(meFd_t* pFd); // declared in usart.c
void console_Init(conCtx_t *pctx_cons){

    static meFd_t meFd_cons;
    meFd_init_console(&meFd_cons);

    memset(pctx_cons, 0, sizeof(conCtx_t));
    memset(pctx_cons->argv, 0, sizeof(pctx_cons->argv));

    DEF_CONS(meCons, CONS_BUFFER_SIZE);

    pctx_cons->pme_sd = &meFd_cons;
    pctx_cons->pBuf_current = pctx_cons->buffer;
    pctx_cons->pBuf_tail = pctx_cons->buffer;
    
    console_Clear(pctx_cons);
    console_Prompt(pctx_cons);
}

void console_Polling(conCtx_t *ctx) {
    if (ctx->is_interactive) {
        com_cmd_desc_t *d = (com_cmd_desc_t *) ctx->com_cmd_desc;
        if (d->fnCon(ctx) != CON_RC_INTERACTIVE) {
            ctx->is_interactive = false;
            console_Prompt(ctx);
        }
    }

    char c = console_Reading(ctx);
    if (ctx->is_interactive && (c!=ECHAP && c!='\3')) // ctrl+c
        return;

    switch (c) {
        case NO_INPUT: break;
        case '\3': /* [ctrl] C */
        case ECHAP:
            console_Echap(ctx);
            break;

        case '\r': /* enter */
            if ( console_entrer(ctx) ) {
                switch (console_match_cmd(ctx))
                {
                case CON_RC_INTERACTIVE:
                    ctx->is_interactive = true;
                    break;
                
                case CON_RC_DONE: /*no break*/
                case CON_RC_BAD_ARG: /*no break*/
                default:
                    ctx->is_interactive = false;
                    console_Prompt(ctx);
                    break;
                }
            }
            break;
        case '\177':
            console_Backspace(ctx);
            break;
        case '\011':
            console_Tabulation(ctx);
            break;
        case LEFT_ARROW:
            console_LeftArrow(ctx);
            break;
        case RIGHT_ARROW:
            console_RightArrow(ctx);
            break;
        case UP_ARROW:
            console_UpArrow(ctx);
            break;
        case DOWN_ARROW:
            console_DownArrow(ctx);
            break;
        case DEBUT:
            console_Home(ctx);
            break;
        case FIN:
            console_End(ctx);
            break;
        case DEL:
            console_Del(ctx);
            break;
        case CTRL_LEFT_ARROW:
            console_CtrlLeftArrow(ctx);
            break;
        case CTRL_RIGHT_ARROW:
            console_CtrlRightArrow(ctx);
            break;

        default:
            console_Carac(ctx, c);
            break;
        }
}

char console_Reading(conCtx_t *ctx)
{

    if (ctx->is_esc && meDelay(ctx->delay_cons)) {
        ctx->is_esc = 0;
        
        if (ctx->index_seq == 1) {
            ctx->index_seq = 0;
            return ECHAP;
        }

        ctx->index_seq = 0;
    }

    char c;
    if (!meFd_Read(ctx->pme_sd, &c, 1))
        return NO_INPUT;

    // printf("[%02X]\r\n", (unsigned char)c);

    if (c == '\x1B') {
        ctx->buff_seq[0] = c;
        ctx->index_seq = 1;
        ctx->is_esc = 1;
        meDelayInit(ctx->delay_cons, ESC_TIMEOUT);
        return NO_INPUT;
    }

    if (ctx->is_esc) {

        if (ctx->index_seq < sizeof(ctx->buff_seq))
            ctx->buff_seq[ctx->index_seq++] = c;
        else {
            ctx->is_esc = 0;
            ctx->index_seq = 0;
            return NO_INPUT;
        }

        for (const cons_sp_char_t *p = cons_sp_char; p->pSeq; p++)
        {
            if (ctx->index_seq <= p->len &&
                memcmp(ctx->buff_seq, p->pSeq, ctx->index_seq) == 0)
            {
                if (ctx->index_seq == p->len) {
                    ctx->is_esc = 0;
                    ctx->index_seq = 0;
                    return p->code;
                }

                return NO_INPUT;
            }
        }

        ctx->is_esc = 0;
        ctx->index_seq = 0;
        return NO_INPUT;
    }

    return c;
}

void console_Prompt(conCtx_t *ctx){

    meFd_Printf(ctx->pme_sd, "%s> ", promptName);
}

void console_ClearLine(conCtx_t *ctx){
    meFd_Print(ctx->pme_sd, "\r\x1B[2K");
}

cons_cmd_rc_t console_parse_cmd(conCtx_t *ctx)
{
    char *p = ctx->pbuf_command; // buffer de commande
    char **argv = ctx->argv; // liste des arguments

    ctx->argc = 0; // compteur d'arguments

    if (*p == '\0') // buffer de commande vide
        return CON_RC_DONE;

    while (*p)
    {
        /* skip spaces */
        while (*p == ' ')
        {
            p = rb_next(ctx->buffer, CONS_BUFFER_SIZE, p);
            if (*p == '\0') // pas d'arguments dans la commande
                return CON_RC_DONE;
        }

        if (ctx->argc >= CONS_ARGC_MAX) // nouvel argument en préparation
           return CON_RC_BAD_ARG;

        argv[ctx->argc++] = p; // stockage pointeur commande

        /* move until space or end */
        while (*p && *p != ' ')
            p = rb_next(ctx->buffer, CONS_BUFFER_SIZE, p);

        if (*p == '\0')
            break;

        p = rb_next(ctx->buffer, CONS_BUFFER_SIZE, p);
    }

    return CON_RC_DONE;
}

cons_cmd_rc_t console_match_cmd(conCtx_t *pCtx)
{
    console_parse_cmd(pCtx);

    if (pCtx->argc == 0)
        return CON_RC_BAD_ARG;

    char *cmd_name = pCtx->argv[0];

    const com_cmd_desc_t *cmd = &__start_console_cmd_list;
    const com_cmd_desc_t *cmd_end = &__end_console_cmd_list;

    while (cmd < cmd_end)
    {
        if (cmd->fnCon && rb_strcmp(pCtx, cmd_name, cmd->name))
        {
            pCtx->com_cmd_desc = cmd;
            return cmd->fnCon(pCtx);
        }

        cmd++;
    }

    cprintf("\rCommande introuvable, utilisez 'help'\r\n");

    return CON_RC_BAD_ARG;
}

cons_cmd_rc_t console_fn_Help(conCtx_t *pCtx){
    const com_cmd_desc_t *cmd = &__start_console_cmd_list;
    const com_cmd_desc_t *cmd_end = &__end_console_cmd_list;

    if (ARGNB(1)) 
    {
        for (; cmd!=cmd_end; cmd++)
        {
            if (cmd->fnCon && ARGEQ(1, cmd->name)) 
            {
                cprintf("\r%10s %60s %s \r\n","| name |","|descritpion|","|usage|");
                cprintf("%10s %40s  %s \r\n\033[0m",cmd->name, cmd->desc, cmd->usage);
                return CON_RC_DONE;
            }
        }

        cprintf("argument inconnu : %s\r\n", pCtx->argv[1]);
        return CON_RC_BAD_ARG;
    }
    
    cprintf("%-10s   %50s   %10s \r\n","| name |","| descritpion |","| usage |");

    cmd = &__start_console_cmd_list;
    while (cmd < cmd_end) {
        if (cmd->fnCon != NULL) {
            cprintf("%-10s   %50s   %-10s \r\n", cmd->name, cmd->desc, cmd->usage);
        }
        cmd++;
    }
    cprintf( VT100_STR_RED );
    return CON_RC_DONE;
}

cons_cmd_rc_t console_fn_Test(conCtx_t *pCtx){
    com_cmd_desc_t *com_cmd_desc = (com_cmd_desc_t *) pCtx->com_cmd_desc;

    cprintf("\033[36m%s\r\n\033[0m",com_cmd_desc->desc);
    return CON_RC_DONE;
}

cons_cmd_rc_t console_fn_Uptime(conCtx_t *pCtx) {
    typedef struct {
        meDelay_t uptime_delay;
        uint64_t uptime_timeout;
    } con_ctx_uptime_t;

    if ( ARGNB(0) )
    {
        cprintf("\rJour : %d, Heure : %d, Minutes : %d, Secondes : %d \r\n", GiveTimeSec()/(24*3600),(GiveTimeSec()/3600)%24, (GiveTimeSec()/60)%60, GiveTimeSec()%60);
        return CON_RC_DONE;

    } else if ( ARGNB(1) )
    {
        if ( ARGEQ(1, "-f") ){
            if( !pCtx->is_interactive ){
                con_ctx_uptime_t* p = CON_CTX_ALLOC(con_ctx_uptime_t);
                pCtx->ctx_mem = p;

                p->uptime_timeout = 1000;
                meDelayInit(&p->uptime_delay, p->uptime_timeout);
            }else {
                con_ctx_uptime_t *p = (con_ctx_uptime_t *) pCtx->ctx_mem;
                if ( !meDelay(&p->uptime_delay) ) return CON_RC_INTERACTIVE;

                cprintf("\rJour : %d, Heure : %d, Minutes : %d, Secondes : %d \r",GiveTimeSec()/(24*3600),(GiveTimeSec()/3600)%24, (GiveTimeSec()/60)%60, GiveTimeSec()%60);
                meDelayInit(&(p->uptime_delay),p->uptime_timeout);
            }
            return CON_RC_INTERACTIVE;
        }else 
            return CON_RC_BAD_ARG;
    } else 
        return CON_RC_BAD_ARG;
}

cons_cmd_rc_t console_fn_Reboot(conCtx_t *ctx) {
    NVIC_SystemReset();
    return CON_RC_DONE;
}

cons_cmd_rc_t console_fn_Echo(conCtx_t *pCtx) {
    bool newline = 0;
    uint8_t start_index = 1;

    if (pCtx->argc == 1) {
        cprintf("\r\n"); 
        return CON_RC_DONE;
    }

    if(!strcmp(pCtx->argv[start_index],"-n")){
        newline = true;
        start_index = 2;
    }

    for (int i = start_index; i < pCtx->argc; i++) {
        cprintf("%s", pCtx->argv[i]);
        if (i < pCtx->argc - 1) {
            cprintf(" "); 
        }
    }
    if (!newline){
        cprintf("\r\n"); 
    } 

    return CON_RC_DONE;
}

#define CHECK_SET_COLOR(_str, _color) if (ARGEQ(1, (_str))) { cprintf(_color); return CON_RC_DONE; }

cons_cmd_rc_t console_fn_Color(conCtx_t *pCtx){
    if ( ARGNB(1) ){
        CHECK_SET_COLOR("black",          VT100_STR_BLACK);
        CHECK_SET_COLOR("red",            VT100_STR_RED);
        CHECK_SET_COLOR("green",          VT100_STR_GREEN);
        CHECK_SET_COLOR("yellow",         VT100_STR_YELLOW);
        CHECK_SET_COLOR("blue",           VT100_STR_BLUE);
        CHECK_SET_COLOR("magenta",        VT100_STR_MAGENTA);
        CHECK_SET_COLOR("cyan",           VT100_STR_CYAN);
        CHECK_SET_COLOR("white",          VT100_STR_WHITE);

        CHECK_SET_COLOR("bright_black",   VT100_STR_BRIGHT_BLACK);
        CHECK_SET_COLOR("bright_red",     VT100_STR_BRIGHT_RED);
        CHECK_SET_COLOR("bright_green",   VT100_STR_BRIGHT_GREEN);
        CHECK_SET_COLOR("bright_yellow",  VT100_STR_BRIGHT_YELLOW);
        CHECK_SET_COLOR("bright_blue",    VT100_STR_BRIGHT_BLUE);
        CHECK_SET_COLOR("bright_magenta", VT100_STR_BRIGHT_MAGENTA);
        CHECK_SET_COLOR("bright_cyan",    VT100_STR_BRIGHT_CYAN);
        CHECK_SET_COLOR("bright_white",   VT100_STR_BRIGHT_WHITE);

        CHECK_SET_COLOR("reset",          VT100_STR_RESET);

        return CON_RC_BAD_ARG;
    } else
        return CON_RC_BAD_ARG;
}

cons_cmd_rc_t console_fn_Clear(conCtx_t *pCtx) {
    console_Clear(pCtx);
    return CON_RC_DONE;
}