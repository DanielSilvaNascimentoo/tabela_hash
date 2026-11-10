#include "palavras.h"
#include <ctype.h>

static char dobrar_latin1(unsigned cp)
{
    if ((cp >= 0xC0 && cp <= 0xC5) || (cp >= 0xE0 && cp <= 0xE5)) return 'a';
    if (cp == 0xC7 || cp == 0xE7) return 'c';
    if ((cp >= 0xC8 && cp <= 0xCB) || (cp >= 0xE8 && cp <= 0xEB)) return 'e';
    if ((cp >= 0xCC && cp <= 0xCF) || (cp >= 0xEC && cp <= 0xEF)) return 'i';
    if (cp == 0xD1 || cp == 0xF1) return 'n';
    if ((cp >= 0xD2 && cp <= 0xD6) || cp == 0xD8 || (cp >= 0xF2 && cp <= 0xF6) || cp == 0xF8) return 'o';
    if ((cp >= 0xD9 && cp <= 0xDC) || (cp >= 0xF9 && cp <= 0xFC)) return 'u';
    if (cp == 0xDD || cp == 0xFD || cp == 0xFF) return 'y';
    return 0;
}

static char classificar(const unsigned char *p, size_t *len)
{
    unsigned char c = p[0];
    if (c < 0x80) {
        *len = 1;
        return isalnum(c) ? (char)tolower(c) : 0;
    }
    size_t n = (c >= 0xF0 && c <= 0xF7) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
    if (c > 0xF7) n = 1;
    for (size_t i = 1; i < n; i++)
        if ((p[i] & 0xC0) != 0x80) { n = 1; break; } 
    *len = n;
    if (n == 2) return dobrar_latin1(((c & 0x1Fu) << 6) | (p[1] & 0x3Fu));
    return 0;
}

size_t proxima_palavra(const char **texto, char *saida, size_t cap)
{
    const unsigned char *p = (const unsigned char *)*texto;
    size_t n = 0, len;
    char c;

    if (cap == 0) return 0;

    while (*p) {                                  
        c = classificar(p, &len);
        p += len;
        if (!c) continue;
        if (n + 1 < cap) saida[n++] = c;          
        while (*p) {                             
            c = classificar(p, &len);
            if (!c) break;
            p += len;
            if (n + 1 < cap) saida[n++] = c;      
        break;
    }
    saida[n] = '\0';
    *texto = (const char *)p;
    return n;
}

size_t normalizar_palavra(const char *entrada, char *saida, size_t cap)
{
    const char *p = entrada;
    return proxima_palavra(&p, saida, cap);
}

HtStatus indexar_palavra(TabelaHash *t, const char *palavra, int linha)
{
    Valor *v = ht_buscar(t, palavra);
    if (v) {                                      
        if (valor_add_ocorrencia(v, linha) != 0) return HT_ERRO;
        v->frequencia++;
        return HT_OK;
    }
    Valor novo = {0};
    novo.frequencia = 1;
    if (valor_add_ocorrencia(&novo, linha) != 0) return HT_ERRO;
    HtStatus st = ht_inserir(t, palavra, &novo);
    valor_liberar(&novo);
    return st;
}

size_t indexar_texto(TabelaHash *t, const char *texto)
{
    char palavra[PALAVRA_MAX + 1];
    const char *p = texto;
    int linha = 1;
    size_t total = 0;

    for (;;) {
        const char *antes = p;
        size_t n = proxima_palavra(&p, palavra, sizeof palavra);
        for (const char *q = antes; q < p; q++)   
            if (*q == '\n') linha++;
        if (n == 0) break;
        if (indexar_palavra(t, palavra, linha) == HT_ERRO) break;
        total++;
    }
    return total;
}
