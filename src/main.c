#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hash_table.h"
#include "palavras.h"

typedef struct { const char *chave; const Valor *valor; } Ref;
typedef struct { Ref *v; size_t n, cap; } Lista;

static void coletar(const char *chave, const Valor *valor, void *ctx)
{
    Lista *l = ctx;
    if (l->n == l->cap) {
        size_t nc = l->cap ? l->cap * 2 : 64;
        Ref *p = realloc(l->v, nc * sizeof *p);
        if (!p) return;
        l->v = p; l->cap = nc;
    }
    l->v[l->n].chave = chave;
    l->v[l->n].valor = valor;
    l->n++;
}

static int cmp_ref(const void *a, const void *b)
{
    return strcmp(((const Ref *)a)->chave, ((const Ref *)b)->chave);
}

static void imprimir_registro(const char *chave, const Valor *v)
{
    printf("  %-20s freq=%d  linhas=[", chave, v->frequencia);
    size_t lim = v->n_ocorrencias < 12 ? v->n_ocorrencias : 12;
    for (size_t i = 0; i < lim; i++) printf("%s%d", i ? ", " : "", v->ocorrencias[i]);
    if (v->n_ocorrencias > lim) printf(", ... +%zu", v->n_ocorrencias - lim);
    printf("]\n");
}

static char *ler_arquivo(const char *nome)
{
    FILE *f = fopen(nome, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);
    if (tam < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)tam + 1);
    if (buf) {
        size_t lidos = fread(buf, 1, (size_t)tam, f);
        buf[lidos] = '\0';
    }
    fclose(f);
    return buf;
}

static void carregar(TabelaHash *t, const char *nome)
{
    char *txt = ler_arquivo(nome);
    if (!txt) { printf("Erro: nao foi possivel ler '%s'.\n", nome); return; }
    size_t antes = t->n;
    size_t palavras = indexar_texto(t, txt);
    printf("'%s': %zu palavras lidas, %zu distintas novas (n=%zu).\n",
           nome, palavras, t->n - antes, t->n);
    free(txt);
}

static void ajuda(void)
{
    puts("Comandos:\n"
         "  carregar <arquivo>            indexa um arquivo de texto\n"
         "  inserir <palavra> [linha]     nova palavra (rejeita duplicata)\n"
         "  indexar <palavra> [linha]     +1 ocorrencia (cria se nao existir)\n"
         "  buscar <palavra>\n"
         "  atualizar <palavra> <freq>    altera a frequencia, mantem as linhas\n"
         "  remover <palavra>\n"
         "  listar                        todas as palavras em ordem alfabetica\n"
         "  estrutura [max_buckets]       exibirEstrutura() (so buckets ocupados)\n"
         "  stats                         alpha, colisoes, cadeias, comparacoes\n"
         "  ajuda | sair");
}

int main(int argc, char **argv)
{
    size_t m = 1009;
    int primeiro_arq = 1;
    if (argc >= 3 && strcmp(argv[1], "-m") == 0) {
        long v = strtol(argv[2], NULL, 10);
        if (v <= 0) { fprintf(stderr, "Capacidade invalida.\n"); return 1; }
        m = (size_t)v;
        primeiro_arq = 3;
    }
    TabelaHash *t = ht_criar(m);
    if (!t) { fprintf(stderr, "Sem memoria.\n"); return 1; }

    printf("Indice de palavras | config 220: E2.2 (lista dupla) + H6 (polinomial) + C1 (encadeamento) | m=%zu\n", m);
    for (int i = primeiro_arq; i < argc; i++) carregar(t, argv[i]);
    int tty = isatty(STDIN_FILENO);
    if (tty) ajuda();

    char linha[1024];
    for (;;) {
        if (tty) { printf("> "); fflush(stdout); }
        if (!fgets(linha, sizeof linha, stdin)) break;

        char cmd[32] = "", a1[512] = "", a2[64] = "";
        if (sscanf(linha, "%31s %511s %63s", cmd, a1, a2) < 1) continue;

        char pal[PALAVRA_MAX + 1] = "";
        size_t pn = a1[0] ? normalizar_palavra(a1, pal, sizeof pal) : 0;

        if (!strcmp(cmd, "sair")) break;
        else if (!strcmp(cmd, "ajuda")) ajuda();
        else if (!strcmp(cmd, "carregar")) { if (a1[0]) carregar(t, a1); else puts("Uso: carregar <arquivo>"); }
        else if (!strcmp(cmd, "listar")) {
            Lista l = {0};
            ht_para_cada(t, coletar, &l);
            qsort(l.v, l.n, sizeof *l.v, cmp_ref);
            for (size_t i = 0; i < l.n; i++) imprimir_registro(l.v[i].chave, l.v[i].valor);
            printf("%zu palavra(s).\n", l.n);
            free(l.v);
        }
        else if (!strcmp(cmd, "estrutura")) ht_exibir_estrutura(t, stdout, 1, a1[0] ? (size_t)atol(a1) : 40);
        else if (!strcmp(cmd, "stats")) {
            HtEstatisticas e = ht_estatisticas(t);
            printf("n=%zu m=%zu alpha=%.3f | buckets ocupados=%zu | maior cadeia=%zu | media cadeia(ocupados)=%.2f\n"
                   "colisoes=%llu | comparacoes=%llu\n",
                   e.n, e.m, e.alpha, e.buckets_ocupados, e.maior_cadeia,
                   e.media_cadeia_ocupados, e.colisoes, e.comparacoes);
        }
        else if (!pn) printf("Informe uma palavra valida (comando '%s').\n", cmd);
        else if (!strcmp(cmd, "inserir") || !strcmp(cmd, "indexar")) {
            int ln = a2[0] ? atoi(a2) : 0;
            HtStatus st;
            if (!strcmp(cmd, "inserir")) {
                Valor v = {0};
                v.frequencia = 1;
                valor_add_ocorrencia(&v, ln);
                st = ht_inserir(t, pal, &v);
                valor_liberar(&v);
                if (st == HT_DUPLICADA) printf("'%s' ja existe (use 'indexar' ou 'atualizar').\n", pal);
            } else st = indexar_palavra(t, pal, ln);
            if (st == HT_OK) printf("OK: '%s' (indice %zu).\n", pal, ht_hash(pal, t->m));
            else if (st == HT_ERRO) puts("Erro ao inserir.");
        }
        else if (!strcmp(cmd, "buscar")) {
            Valor *v = ht_buscar(t, pal);
            if (v) imprimir_registro(pal, v); else printf("'%s' nao encontrada.\n", pal);
        }
        else if (!strcmp(cmd, "atualizar")) {
            Valor *v = ht_buscar(t, pal);
            if (!v) printf("'%s' nao encontrada.\n", pal);
            else if (!a2[0]) puts("Uso: atualizar <palavra> <frequencia>");
            else {
                Valor novo;
                if (valor_copiar(&novo, v) == 0) {
                    novo.frequencia = atoi(a2);
                    ht_atualizar(t, pal, &novo);
                    valor_liberar(&novo);
                    printf("'%s' atualizada.\n", pal);
                }
            }
        }
        else if (!strcmp(cmd, "remover")) {
            HtStatus st = ht_remover(t, pal);
            printf(st == HT_OK ? "'%s' removida.\n" : "'%s' nao encontrada.\n", pal);
        }
        else printf("Comando desconhecido: %s (digite 'ajuda').\n", cmd);
    }
    ht_destruir(t);
    return 0;
}
