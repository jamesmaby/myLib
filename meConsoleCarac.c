#include "meConsoleCarac.h"
#include "meConsole.h"
#include "meMalloc.h"

/* Remplace la ligne courante (ring + affichage) par s */
static void console_SetLine(conCtx_t *ctx, const char *s)
{
    char *pb = ctx->pBuf_current;
    uint16_t len = 0;

    while (s[len] && len < CONS_BUFFER_SIZE - 2) {
        *pb = s[len++];
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }
    *pb = '\0';
    pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    *pb = '\0';

    ctx->index = len;

    console_ClearLine(ctx);
    console_Prompt(ctx);
    meFd_Printf(ctx->pme_sd, "%s", s);
}


/* Copie dans out la k-ième commande la plus récente (k = 1 : la dernière).
 * false si elle n'existe pas ou si elle semble écrasée. */
static bool console_HistGet(conCtx_t *ctx, uint8_t k, char *out)
{
    con_history_ctx_t *h = &ctx->history_ctx;
    char    *pb   = ctx->pBuf_current;      /* = queue du ring */
    uint16_t dist = 0;
    uint16_t len  = 0;

    for (uint8_t i = 1; i <= k; i++) {
        len = h->bufHistCmdSz[(h->histIdx - i) & (CONS_HISTORY_NB - 1)];
        if (len == 0)
            return false;

        dist += len + 1;                    /* caractères + '\0' */
        for (uint16_t j = 0; j <= len; j++)
            pb = rb_prev(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }

    /* la ligne rappelée écrira len+2 octets à la queue : elle ne doit pas
     * écraser la commande source */
    if (dist + len + 2 > CONS_BUFFER_SIZE)
        return false;

    for (uint16_t j = 0; j < len; j++) {
        if (*pb == '\0')
            return false;
        out[j] = *pb;
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }
    if (*pb != '\0')
        return false;

    out[len] = '\0';
    return true;
}

void console_Clear(conCtx_t *ctx){
    meFd_Print(ctx->pme_sd, "\x1B[2J\x1B[H");
}

bool console_entrer(conCtx_t *ctx) {
    if (ctx->is_interactive)
        return false;

    /*chaîne de caractère vide*/
    if ( rb_lenBuff(ctx) == 0 ) {
        meFd_Printf(ctx->pme_sd, JUMP_LINE);
        console_Prompt(ctx);
        return false;
    }
    
    meFd_Printf(ctx->pme_sd, JUMP_LINE);
    con_history_ctx_t* phist = &ctx->history_ctx;

    if ( rb_lenBuff(ctx) == 0 ) {
        ctx->history_ctx.histPressNb = 0;
        meFd_Printf(ctx->pme_sd, JUMP_LINE);
        console_Prompt(ctx);
        return false;
    }
    ctx->pbuf_command = ctx->pBuf_current;

    phist->bufHistCmdSz[phist->histIdx] = 0;
    
    // Trouver le '\0' de la chaîne courante
    char *pb = ctx->pBuf_current;
    while (*pb) {
        phist->bufHistCmdSz[phist->histIdx]++;
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }

    // La nouvelle chaîne commence juste après
    pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    *pb = '\0';

    /* préparation nouvelle commande */
    char *pb_next = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    *pb_next = '\0';

    phist->histIdx++;
    phist->histIdx &= (CONS_HISTORY_NB-1);
    phist->histPressNb = 0;

    ctx->pBuf_current = pb;
    ctx->pBuf_tail    = pb;
    ctx->index = 0;

    return true;
}

void console_Backspace(conCtx_t *ctx)
{
    if (ctx->index <= 0)
        return;

    if (!rb_lenBuff(ctx))
        return;

    ctx->index--;

    char *pidx = ctx->pBuf_current;
    for (uint16_t i = 0; i < ctx->index; i++)
        pidx = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pidx);

    char *p = pidx;
    while (*p) {
        char *next = rb_next(ctx->buffer, CONS_BUFFER_SIZE, p);
        *p = *next;
        if (*next == '\0') break;
        p = next;
    }

    /* Recule d'un cran puis supprime le caractère visuellement */
    meFd_Print(ctx->pme_sd, "\x1B[D\x1B[1P");
}

void console_LeftArrow(conCtx_t *ctx){
    if( ctx->index <= 0)
        return;

    ctx->index--;
    meFd_Print(ctx->pme_sd, "\x1B[D");
}

void console_RightArrow(conCtx_t *ctx){
    if( ctx->index >= rb_lenBuff(ctx))
        return;

    ctx->index++;
    meFd_Print(ctx->pme_sd, "\x1B[C");
}

void console_UpArrow(conCtx_t *ctx)
{
    con_history_ctx_t *h = &ctx->history_ctx;
    char str[CONS_BUFFER_SIZE];

    if (h->histPressNb >= CONS_HISTORY_NB)
        return;
    if (!console_HistGet(ctx, h->histPressNb + 1, str))
        return;

    h->histPressNb++;
    console_SetLine(ctx, str);
}

void console_DownArrow(conCtx_t *ctx)
{
    con_history_ctx_t *h = &ctx->history_ctx;
    char str[CONS_BUFFER_SIZE];

    if (h->histPressNb == 0)
        return;

    h->histPressNb--;
    if (h->histPressNb == 0 || !console_HistGet(ctx, h->histPressNb, str)) {
        h->histPressNb = 0;
        str[0] = '\0';
    }
    console_SetLine(ctx, str);
}

void console_Echap(conCtx_t *ctx){
    if (ctx->is_interactive && ctx->ctx_mem) {
        meFree(ctx->ctx_mem);
        ctx->ctx_mem = NULL;
    }

    ctx->history_ctx.histPressNb = 0;

    /* J'efface la commande en cours */
    char* pb = ctx->pBuf_current;
    while( *pb ){
        *pb = '\0';
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }

    ctx->is_interactive = false;
    ctx->index = 0; // repart à 0

    meFd_Print(ctx->pme_sd, "\r\n");
    console_Prompt(ctx);
}

void console_Home(conCtx_t *ctx){
    ctx->index = 0;

    char reposition[10];
    meFd_Sprintf(reposition, sizeof(reposition), "\x1B[%dG", LEN_PROMPT);
    meFd_Print(ctx->pme_sd, reposition);

}

void console_End(conCtx_t *ctx){
    ctx->index = rb_lenBuff(ctx);

    char reposition[10];
    meFd_Sprintf(reposition, sizeof(reposition), "\x1B[%dG", rb_lenBuff(ctx)+LEN_PROMPT);
    meFd_Print(ctx->pme_sd, reposition);
}

void console_Carac(conCtx_t *ctx, char c)
{
    if (ctx->is_interactive)
        return;

    uint16_t len = rb_lenBuff(ctx);

    if (len >= CONS_BUFFER_SIZE - 1)
        return;

    char *pend = ctx->pBuf_current;
    for (uint16_t i = 0; i < len; i++)
        pend = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pend);

    char *pidx = ctx->pBuf_current;
    for (uint16_t i = 0; i < ctx->index; i++)
        pidx = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pidx);

    char *pnew_end = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pend);

    char *pcur = pend;
    while (true) {
        char *pnext = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pcur);
        char *pprev = rb_prev(ctx->buffer, CONS_BUFFER_SIZE, pcur);
        *pnext = *pcur;
        if (pcur == pidx) break;
        pcur = pprev;
    }
    *pidx = c;
    *pnew_end = '\0';
    ctx->index++;

    if (ctx->index == len + 1) { /* Insertion en fin : le terminal gère l'avance du curseur */
        meFd_Printf(ctx->pme_sd, "%c", c);
    } else {
        meFd_Printf(ctx->pme_sd, "\x1B[1@%c", c); /* Insertion en milieu : décaler visuellement puis écrire */
    }
}

void console_Del(conCtx_t *ctx)
{
    uint16_t len = rb_lenBuff(ctx);

    if (ctx->index >= len)
        return;

    char *pidx = ctx->pBuf_current;
    for (uint16_t i = 0; i < ctx->index; i++)
        pidx = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pidx);

    char *pcur = pidx;
    while (*pcur) {
        char *pnext = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pcur);
        *pcur = *pnext;
        if (*pnext == '\0') break;
        pcur = pnext;
    }

    /* Supprime le caractère sous le curseur sans le déplacer */
    meFd_Print(ctx->pme_sd, "\x1B[1P");
}

void console_Tabulation(conCtx_t *ctx)
{
    char input[CONS_BUFFER_SIZE];
    rb_to_string(ctx, input, sizeof(input));
    uint16_t input_len = strlen(input);

    if (input_len == 0)
        return;

    /* "adc " -> chercher sur "adc", mais garder le buffer intact */
    bool trailing_space = (input[input_len - 1] == ' ');
    uint16_t cmp_len = trailing_space ? input_len - 1 : input_len;

    if (cmp_len == 0)
        return;

    const com_cmd_desc_t *p    = &__start_console_cmd_list;
    const com_cmd_desc_t *pend = &__end_console_cmd_list;

    const com_cmd_desc_t *first_match = NULL;
    uint16_t found = 0;
    bool exact = false;

    while (p != pend) {

        /* Correspondance exacte sur le nom seul (sans l'espace) */
        if (!strncmp(p->name, input, cmp_len) && p->name[cmp_len] == '\0') {
            exact = true;
            first_match = p;
            found = 1;
            break;
        }

        /* Correspondance par préfixe */
        if (!strncmp(p->name, input, cmp_len)) {
            found++;
            if (first_match == NULL)
                first_match = p;
        }
        p++;
    }

    if (found == 0 || first_match == NULL)
        return;

    /* --- "adc " + TAB : afficher l'usage sans toucher au buffer --- */
    if (trailing_space && exact) {
        meFd_Printf(ctx->pme_sd, "\r\n%s\r\n", first_match->usage);
        console_Prompt(ctx);
        meFd_Printf(ctx->pme_sd, "%s", input);           /* restitue "adc " */
        meFd_Printf(ctx->pme_sd, "\x1B[%dG", ctx->index + LEN_PROMPT);
        return;
    }

    /* --- Exact ou unique préfixe : compléter + usage --- */
    if (exact || found == 1) {

        char *pcur = ctx->pBuf_current;
        const char *s = first_match->name;
        while (*s) {
            *pcur = *s++;
            pcur = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pcur);
        }
        *pcur = ' ';
        pcur = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pcur);
        *pcur = '\0';

        ctx->index        = strlen(first_match->name) + 1;

        console_ClearLine(ctx);
        console_Prompt(ctx);
        meFd_Printf(ctx->pme_sd, "%s ", first_match->name);
        meFd_Printf(ctx->pme_sd, "\r\n%s\r\n", first_match->usage);

        console_Prompt(ctx);
        meFd_Printf(ctx->pme_sd, "%s ", first_match->name);
        meFd_Printf(ctx->pme_sd, "\x1B[%dG", ctx->index + LEN_PROMPT);
        return;
    }

    /* --- Plusieurs correspondances : lister --- */
    meFd_Printf(ctx->pme_sd, "\r\n");
    p = &__start_console_cmd_list;
    while (p != pend) {
        if (!strncmp(p->name, input, cmp_len))
            meFd_Printf(ctx->pme_sd, "%s  ", p->name);
        p++;
    }
    meFd_Printf(ctx->pme_sd, "\r\n");

    console_Prompt(ctx);
    meFd_Printf(ctx->pme_sd, "%s", input);
    meFd_Printf(ctx->pme_sd, "\x1B[%dG", ctx->index + LEN_PROMPT);
}

void console_CtrlLeftArrow(conCtx_t *ctx)
{
    if (ctx->index == 0) return;

    uint16_t old_index = ctx->index;

    /*je récupère le pointeur du buffeur courant*/
    char *pb = ctx->pBuf_current;
    for (uint16_t i = 0; i < ctx->index - 1; i++)
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);

    /*je me décale des espaces à droite*/
    while (ctx->index > 0 && *pb == ' ') {
        ctx->index--;
        if (ctx->index > 0)
            pb = rb_prev(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }

    /*je me décale des caractères à droite*/
    while (ctx->index > 0 && *pb != ' ') {
        ctx->index--;
        if (ctx->index > 0)
            pb = rb_prev(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }

    uint16_t delta = old_index - ctx->index;
    if (delta > 0)
        meFd_Printf(ctx->pme_sd, "\x1B[%dD", delta);  // recule de delta colonnes
}

void console_CtrlRightArrow(conCtx_t *ctx)
{
    uint16_t len = rb_lenBuff(ctx);
    if (ctx->index == len) return;

    uint16_t old_index = ctx->index;

    /*je récupère le pointeur du buffeur courant*/
    char *pb = ctx->pBuf_current;
    for (uint16_t i = 0; i < ctx->index; i++)
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);

    /*je me décale des espaces à droite*/
    while (ctx->index < len && *pb == ' ') {
        ctx->index++;
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }

    /*je me décale des caractères à droite*/
    while (ctx->index < len && *pb != ' ') {
        ctx->index++;
        pb = rb_next(ctx->buffer, CONS_BUFFER_SIZE, pb);
    }

    uint16_t delta = ctx->index - old_index;
    if (delta > 0)
        meFd_Printf(ctx->pme_sd, "\x1B[%dC", delta);  // avance de delta colonnes
}