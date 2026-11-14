#define _POSIX_C_SOURCE 200809L
/*
 * bench.c - Analise experimental (config 220: E2.2, H6, C1, A4)
 * Cenarios: n = 100, 1.000, 10.000 palavras; tres capacidades por cenario
 * (m ~ n/2, n, 2n, primos); 5 repeticoes com sementes distintas.
 * Uso: ./bench [repeticoes] [arquivo.csv]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_table.h"

#define TAM_PAL 8   /* 7 letras + NUL */

static double agora_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static size_t proximo_primo(size_t n)
{
    for (;; n++) {
        if (n < 2) continue;
        int primo = 1;
        for (size_t d = 2; d * d <= n; d++) if (n % d == 0) { primo = 0; break; }
        if (primo) return n;
    }
}

static unsigned xorshift(unsigned *s)
{
    unsigned x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

/* Palavras de 7 letras: inicial + 2 letras aleatorias + codigo bijetivo do indice
 * (4 letras, embaralhado). Inicial distinta => conjuntos presente/ausente disjuntos;
 * codigo do indice => palavras distintas dentro de cada conjunto. */
static void gerar(char (*dst)[TAM_PAL], size_t n, char inicial, unsigned *estado)
{
    for (size_t i = 0; i < n; i++) {
        unsigned cod = (unsigned)((i * 40503u + 12345u) % 456976u);   /* 26^4 */
        char *w = dst[i];
        w[0] = inicial;
        w[1] = (char)('a' + xorshift(estado) % 26);
        w[2] = (char)('a' + xorshift(estado) % 26);
        for (int k = 6; k >= 3; k--) { w[k] = (char)('a' + cod % 26); cod /= 26; }
        w[7] = '\0';
    }
}

typedef struct {
    double colisoes, ins_ms, ok_ms, aus_ms, cmp_ok, cmp_aus, maior, media_cad, ocupados;
} Acum;

int main(int argc, char **argv)
{
    int reps = argc > 1 ? atoi(argv[1]) : 5;
    if (reps < 1) reps = 5;
    FILE *csv = argc > 2 ? fopen(argv[2], "w") : NULL;
    if (csv) fprintf(csv, "registros,capacidade,alpha,colisoes,ins_ms,busca_ok_ms,busca_aus_ms,cmp_ok,cmp_aus,maior_cadeia,media_cadeia\n");

    const size_t ns[3] = { 100, 1000, 10000 };
    printf("Config 220 (E2.2, H6, C1, A4) | media de %d repeticoes por cenario\n", reps);
    printf("%8s %7s %6s %9s %10s %12s %13s %9s %9s %7s\n", "Registros", "Cap.", "alpha", "Colisoes",
           "Ins.(ms)", "Busca ok(ms)", "Busca aus(ms)", "cmp/ok", "cmp/aus", "cadeia");

    for (int ci = 0; ci < 3; ci++) {
        size_t n = ns[ci];
        size_t caps[3] = { proximo_primo(n / 2), proximo_primo(n), proximo_primo(2 * n) };
        for (int cj = 0; cj < 3; cj++) {
            size_t m = caps[cj];
            Acum a = {0};
            for (int r = 0; r < reps; r++) {
                unsigned semente = 2463534242u + 7919u * (unsigned)r;
                char (*pres)[TAM_PAL] = malloc(n * TAM_PAL);
                char (*aus)[TAM_PAL] = malloc(n * TAM_PAL);
                gerar(pres, n, 'p', &semente);
                gerar(aus, n, 'q', &semente);

                TabelaHash *t = ht_criar(m);
                Valor v = { 1, NULL, 0, 0 };

                double t0 = agora_ms();
                for (size_t i = 0; i < n; i++)
                    if (ht_inserir(t, pres[i], &v) != HT_OK) { fprintf(stderr, "falha na insercao\n"); return 1; }
                double t1 = agora_ms();
                a.ins_ms += t1 - t0;
                a.colisoes += (double)t->colisoes;

                size_t passes = 100000 / n; if (passes < 1) passes = 1;   /* amostra maior p/ n pequeno */
                volatile size_t achou = 0;

                ht_zerar_contadores(t);
                t0 = agora_ms();
                for (size_t p = 0; p < passes; p++)
                    for (size_t i = 0; i < n; i++) achou += ht_buscar(t, pres[i]) != NULL;
                t1 = agora_ms();
                if (achou != n * passes) { fprintf(stderr, "busca com sucesso falhou\n"); return 1; }
                a.ok_ms += (t1 - t0) / (double)passes;
                a.cmp_ok += (double)t->comparacoes / (double)(n * passes);

                ht_zerar_contadores(t);
                achou = 0;
                t0 = agora_ms();
                for (size_t p = 0; p < passes; p++)
                    for (size_t i = 0; i < n; i++) achou += ht_buscar(t, aus[i]) != NULL;
                t1 = agora_ms();
                if (achou != 0) { fprintf(stderr, "busca sem sucesso falhou\n"); return 1; }
                a.aus_ms += (t1 - t0) / (double)passes;
                a.cmp_aus += (double)t->comparacoes / (double)(n * passes);

                HtEstatisticas e = ht_estatisticas(t);
                a.maior += (double)e.maior_cadeia;
                a.media_cad += e.media_cadeia_ocupados;
                a.ocupados += (double)e.buckets_ocupados;

                ht_destruir(t);
                free(pres); free(aus);
            }
            double R = (double)reps;
            printf("%8zu %7zu %6.2f %9.1f %10.4f %12.4f %13.4f %9.2f %9.2f %7.1f\n", n, m, (double)n / (double)m,
                   a.colisoes / R, a.ins_ms / R, a.ok_ms / R, a.aus_ms / R, a.cmp_ok / R, a.cmp_aus / R, a.maior / R);
            if (csv)
                fprintf(csv, "%zu,%zu,%.4f,%.2f,%.5f,%.5f,%.5f,%.3f,%.3f,%.2f,%.3f\n", n, m, (double)n / (double)m,
                        a.colisoes / R, a.ins_ms / R, a.ok_ms / R, a.aus_ms / R, a.cmp_ok / R, a.cmp_aus / R,
                        a.maior / R, a.media_cad / R);
        }
    }
    if (csv) fclose(csv);
    return 0;
}
