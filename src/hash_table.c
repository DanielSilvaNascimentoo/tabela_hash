#include "hash_table.h"
#include <stdlib.h>
#include <string.h>

void valor_liberar(Valor *v)
{
    if (!v) return;
    free(v->ocorrencias);
    v->ocorrencias = NULL;
    v->n_ocorrencias = v->cap_ocorrencias = 0;
}

int valor_add_ocorrencia(Valor *v, int linha)
{
    if (v->n_ocorrencias == v->cap_ocorrencias) {
        size_t nova = v->cap_ocorrencias ? v->cap_ocorrencias * 2 : 4;
        int *p = realloc(v->ocorrencias, nova * sizeof *p);
        if (!p) return -1;
        v->ocorrencias = p;
        v->cap_ocorrencias = nova;
    }
    v->ocorrencias[v->n_ocorrencias++] = linha;
    return 0;
}

int valor_copiar(Valor *dst, const Valor *src)
{
    dst->frequencia = src->frequencia;
    dst->ocorrencias = NULL;
    dst->n_ocorrencias = dst->cap_ocorrencias = 0;
    if (src->n_ocorrencias > 0) {
        dst->ocorrencias = malloc(src->n_ocorrencias * sizeof(int));
        if (!dst->ocorrencias) return -1;
        memcpy(dst->ocorrencias, src->ocorrencias, src->n_ocorrencias * sizeof(int));
        dst->n_ocorrencias = dst->cap_ocorrencias = src->n_ocorrencias;
    }
    return 0;
}

size_t ht_hash(const char *chave, size_t m)
{
    unsigned long long h = 0;
    for (const unsigned char *p = (const unsigned char *)chave; *p; ++p)
        h = (h * HT_BASE_POLINOMIAL + *p) % m;
    return (size_t)h;
}

static char *copiar_texto(const char *s)
{
    size_t n = strlen(s) + 1;
    char *c = malloc(n);
    if (c) memcpy(c, s, n);
    return c;
}

static void liberar_no(No *no)
{
    free(no->chave);
    valor_liberar(&no->valor);
    free(no);
}

TabelaHash *ht_criar(size_t m)
{
    if (m == 0) return NULL;
    TabelaHash *t = calloc(1, sizeof *t);
    if (!t) return NULL;
    t->buckets = calloc(m, sizeof *t->buckets);
    if (!t->buckets) { free(t); return NULL; }
    t->m = m;
    return t;
}

void ht_destruir(TabelaHash *t)
{
    if (!t) return;
    for (size_t i = 0; i < t->m; i++) {
        No *p = t->buckets[i];
        while (p) { No *prox = p->prox; liberar_no(p); p = prox; }
    }
    free(t->buckets);
    free(t);
}

static No *localizar(TabelaHash *t, const char *chave, size_t idx)
{
    for (No *p = t->buckets[idx]; p; p = p->prox) {
        t->comparacoes++;
        if (strcmp(p->chave, chave) == 0) return p;
    }
    return NULL;
}

HtStatus ht_inserir(TabelaHash *t, const char *chave, const Valor *valor)
{
    if (!t || !chave || !*chave || !valor) return HT_ERRO;

    size_t idx = ht_hash(chave, t->m);
    if (localizar(t, chave, idx)) return HT_DUPLICADA;   
    No *no = calloc(1, sizeof *no);
    if (!no) return HT_ERRO;
    no->chave = copiar_texto(chave);
    if (!no->chave || valor_copiar(&no->valor, valor) != 0) {
        free(no->chave);
        free(no);
        return HT_ERRO;
    }

    if (t->buckets[idx]) t->colisoes++;

    no->prox = t->buckets[idx];
    if (no->prox) no->prox->ant = no;
    t->buckets[idx] = no;
    t->n++;
    return HT_OK;
}

Valor *ht_buscar(TabelaHash *t, const char *chave)
{
    if (!t || !chave || !*chave) return NULL;
    No *p = localizar(t, chave, ht_hash(chave, t->m));
    return p ? &p->valor : NULL;
}

HtStatus ht_atualizar(TabelaHash *t, const char *chave, const Valor *valor)
{
    if (!t || !chave || !*chave || !valor) return HT_ERRO;
    No *p = localizar(t, chave, ht_hash(chave, t->m));
    if (!p) return HT_NAO_ENCONTRADA;

    Valor copia;
    if (valor_copiar(&copia, valor) != 0) return HT_ERRO;
    valor_liberar(&p->valor);
    p->valor = copia;                 
    return HT_OK;
}

HtStatus ht_remover(TabelaHash *t, const char *chave)
{
    if (!t || !chave || !*chave) return HT_ERRO;
    size_t idx = ht_hash(chave, t->m);
    No *p = localizar(t, chave, idx);
    if (!p) return HT_NAO_ENCONTRADA;

    if (p->ant) p->ant->prox = p->prox;
    else        t->buckets[idx] = p->prox;
    if (p->prox) p->prox->ant = p->ant;

    liberar_no(p);
    t->n--;
    return HT_OK;
}

HtEstatisticas ht_estatisticas(const TabelaHash *t)
{
    HtEstatisticas e = {0};
    e.n = t->n;
    e.m = t->m;
    e.alpha = (double)t->n / (double)t->m;
    e.colisoes = t->colisoes;
    e.comparacoes = t->comparacoes;
    size_t soma = 0;
    for (size_t i = 0; i < t->m; i++) {
        size_t len = 0;
        for (const No *p = t->buckets[i]; p; p = p->prox) len++;
        if (len) {
            e.buckets_ocupados++;
            soma += len;
            if (len > e.maior_cadeia) e.maior_cadeia = len;
        }
    }
    e.media_cadeia_ocupados = e.buckets_ocupados ? (double)soma / (double)e.buckets_ocupados : 0.0;
    return e;
}

void ht_zerar_contadores(TabelaHash *t)
{
    t->colisoes = 0;
    t->comparacoes = 0;
}

void ht_exibir_estrutura(const TabelaHash *t, FILE *out, int apenas_ocupados, size_t max_buckets)
{
    HtEstatisticas e = ht_estatisticas(t);
    fprintf(out, "Tabela hash: m=%zu n=%zu alpha=%.3f ocupados=%zu maior_cadeia=%zu colisoes=%llu\n",
            e.m, e.n, e.alpha, e.buckets_ocupados, e.maior_cadeia, e.colisoes);

    size_t impressos = 0, omitidos = 0;
    for (size_t i = 0; i < t->m; i++) {
        const No *p = t->buckets[i];
        if (apenas_ocupados && !p) continue;
        if (max_buckets && impressos >= max_buckets) { omitidos++; continue; }
        impressos++;

        fprintf(out, "  [%4zu] ", i);
        if (!p) { fprintf(out, "(vazio)\n"); continue; }
        fprintf(out, "NULL");
        size_t len = 0;
        for (; p; p = p->prox, len++)
            fprintf(out, " <-> \"%s\"(f=%d)", p->chave, p->valor.frequencia);
        fprintf(out, " <-> NULL  (tam=%zu)\n", len);
    }
    if (omitidos) fprintf(out, "  ... %zu bucket(s) omitido(s)\n", omitidos);
}

int ht_verificar_integridade(const TabelaHash *t)
{
    size_t total = 0;
    for (size_t i = 0; i < t->m; i++) {
        const No *p = t->buckets[i];
        if (p && p->ant != NULL) return 0;                    
        for (; p; p = p->prox) {
            if (p->prox && p->prox->ant != p) return 0;       
            if (ht_hash(p->chave, t->m) != i) return 0;       
            total++;
        }
    }
    return total == t->n;
}

void ht_para_cada(const TabelaHash *t,
                  void (*fn)(const char *, const Valor *, void *), void *ctx)
{
    for (size_t i = 0; i < t->m; i++)
        for (const No *p = t->buckets[i]; p; p = p->prox)
            fn(p->chave, &p->valor, ctx);
}