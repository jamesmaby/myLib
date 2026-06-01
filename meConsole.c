#include "meConsole.h"
#include "meConsoleCarac.h"
#include "stm32f0xx.h"
#include "system.h"

#define CTX_FN(_ctx, _ctx_fn)                                   \
if (sizeof(_ctx_fn) > sizeof(_ctx->ctx_mem)) {                  \
    printf("Erreur: mémoire insuffisante pour le contexte\r\n"); \
    return CON_RC_DONE;                                         \
}                                                               \
_ctx_fn *p = (_ctx_fn *) _ctx->ctx_mem; 

#define DEF_SEQ( _seq, _code) { _seq, sizeof(_seq) - 1, _code }


#define VT100_STR_BLUE "\033[36m"
#define VT100_STR_RED  "\033[31m"


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

void meFd_init_console(meFd_t *); // declared in usart.c
void console_Init(conCtx_t *pctx_cons){

    static meFd_t meFd_cons;
    meFd_init_console(&meFd_cons);

    memset(pctx_cons, 0, sizeof(conCtx_t));
    memset(pctx_cons->argv, 0, sizeof(pctx_cons->argv));

    DEF_CONS(meCons, CONS_BUFFER_SIZE);

    pctx_cons->pme_sd = &meFd_cons;
    pctx_cons->pBuf_current = pctx_cons->buffer;
    
    console_Clear(pctx_cons);
    console_Prompt(pctx_cons);
}

void console_Polling(conCtx_t *ctx) {
    char c = console_Reading(ctx);

    if (c == NO_INPUT){
        if (ctx->is_interactive) {
            com_cmd_desc_t *com_cmd_desc = (com_cmd_desc_t *) ctx->com_cmd_desc;
            com_cmd_desc->fnCon(ctx);
        }
        return;
    }

    if (ctx->is_interactive && (c!=ECHAP && c!='\3'))
        return;

    switch (c) {
        case '\3': /* [ctrl] C */
        case ECHAP:
            console_Echap(ctx);
            break;
        case '\r': /* enter */
            if (console_entrer(ctx)) console_match_cmd(ctx);
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
        meDelayInit(ctx->delay_cons, TIMEOUT);
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

        if (ctx->argc >= CON_ARGC_MAX) // nouvel argument en préparation
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

cons_cmd_rc_t console_match_cmd(conCtx_t *ctx)
{
    console_parse_cmd(ctx);

    if (ctx->argc == 0)
        return CON_RC_BAD_ARG;

    char *cmd_name = ctx->argv[0];

    const com_cmd_desc_t *cmd = &__start_console_cmd_list;
    const com_cmd_desc_t *cmd_end = &__end_console_cmd_list;

    while (cmd < cmd_end)
    {
        if (cmd->fnCon && rb_strcmp(ctx, cmd_name, cmd->name))
        {
            ctx->com_cmd_desc = cmd;
            return cmd->fnCon(ctx);
        }

        cmd++;
    }

    meFd_Printf(ctx->pme_sd, "\rCommande introuvable, utilisez 'help'\r\n");
    console_Prompt(ctx);

    return CON_RC_BAD_ARG;
}

cons_cmd_rc_t console_fn_Help(conCtx_t *ctx){
    const com_cmd_desc_t *cmd = &__start_console_cmd_list;
    const com_cmd_desc_t *cmd_end = &__end_console_cmd_list;

    if (ARGNB(1)) 
    {
        for (; cmd!=cmd_end; cmd++)
        {
            if (cmd->fnCon && !strcmp(ctx->argv[1], cmd->name)) 
            {
                printf("\r%10s %60s %s \r\n","| name |","|descritpion|","|usage|");
                printf("%10s %40s  %s \r\n\033[0m",cmd->name, cmd->desc, cmd->usage);
                console_Prompt(ctx);
                return CON_RC_DONE;
            }
        }

        printf("argument inconnu : %s\r\n", ctx->argv[1]);
        console_Prompt(ctx);
        return CON_RC_BAD_ARG;
    }
    
    printf("%-10s   %50s   %10s \r\n","| name |","| descritpion |","| usage |");

    cmd = &__start_console_cmd_list;
    while (cmd < cmd_end) {
        if (cmd->fnCon != NULL) {
            printf("%-10s   %50s   %-10s \r\n", cmd->name, cmd->desc, cmd->usage);
        }
        cmd++;
    }
    printf( VT100_STR_RED );
    console_Prompt(ctx);
    return CON_RC_DONE;
}

cons_cmd_rc_t console_fn_Test(conCtx_t *ctx){
    com_cmd_desc_t *com_cmd_desc = (com_cmd_desc_t *) ctx->com_cmd_desc;

    printf("\033[36m%s\r\n\033[0m",com_cmd_desc->desc);
    console_Prompt(ctx);
    return CON_RC_DONE;
}

cons_cmd_rc_t console_fn_Uptime(conCtx_t *ctx) {
    
    if (ctx->is_interactive) goto interactive;

    typedef struct {
        meDelay_t uptime_delay;
        uint64_t uptime_timeout;
    } con_ctx_uptime_t;

    CTX_FN(ctx,con_ctx_uptime_t);
    p->uptime_timeout = 1000;

    bool format_flag = false;

    for (int i = 1; i < ctx->argc; i++) {  // Commence à 1 (ignore la commande elle-même)
        if (strcmp(ctx->argv[i], "-f") == 0) {
            format_flag = true;
        } else {
            printf("argument inconnu : %s\r\n", ctx->argv[i]);
            console_Prompt(ctx);
            return CON_RC_BAD_ARG;
        }
    }

    meDelayInit(&p->uptime_delay, p->uptime_timeout);

    if (format_flag) {
        if (ctx->is_interactive == 0){
            ctx->is_interactive = true;
        }else{
            interactive:
            con_ctx_uptime_t *p = (con_ctx_uptime_t *) ctx->ctx_mem;
            if (meDelay(&(p->uptime_delay))){
                printf("\rJour : %d, Heure : %d, Minutes : %d, Secondes : %d \r",GiveTimeSec()/(24*3600),(GiveTimeSec()/3600)%24, (GiveTimeSec()/60)%60, GiveTimeSec()%60);
                meDelayInit(&(p->uptime_delay),p->uptime_timeout);
            } else return INTERACTIVE;
        }
    } else {
        printf("\rJour : %d, Heure : %d, Minutes : %d, Secondes : %d \r\n",GiveTimeSec()/(24*3600),(GiveTimeSec()/3600)%24, (GiveTimeSec()/60)%60, GiveTimeSec()%60);
        console_Prompt(ctx);
    }

    return INTERACTIVE;
}

cons_cmd_rc_t console_fn_Reboot(conCtx_t *ctx) {
    NVIC_SystemReset();
    console_Prompt(ctx);
    return CON_RC_DONE;
}

cons_cmd_rc_t console_fn_Echo(conCtx_t *ctx) {
    bool newline = 0;
    uint8_t start_index = 1;

    if (ctx->argc == 1) {
        printf("\r\n"); 
        return CON_RC_DONE;
    }

    if(!strcmp(ctx->argv[start_index],"-n")){
        newline = true;
        start_index = 2;
    }

    for (int i = start_index; i < ctx->argc; i++) {
        printf("%s", ctx->argv[i]);
        if (i < ctx->argc - 1) {
            printf(" "); 
        }
    }
    if (!newline){
        printf("\r\n"); 
        console_Prompt(ctx);
    } 

    return CON_RC_DONE;
}

cons_cmd_rc_t console_fn_Clear(conCtx_t *ctx) {
    console_Clear(ctx);
    console_Prompt(ctx);
    return CON_RC_DONE;
}