/*
 * test_hash.c - Testes de correcao obrigatorios (config 220: E2.2, H6, C1, A4)
 * Saida: para cada caso, entradas, resultado esperado, obtido e situacao.
 */
#include <stdio.h>
#include <string.h>
#include "hash_table.h"
#include "palavras.h"

static int total = 0, aprovados = 0;

static void caso(const char *nome, const char *entradas, const char *esperado,
                 const char *obtido, int ok)
{
    total++;
    if (ok) aprovados++;
    printf("[%s] %s\n    entradas: %s\n    esperado: %s\n    obtido  : %s\n\n",
           ok ? "APROVADO " : "REPROVADO", nome, entradas, esperado, obtido);
}

static Valor val(int freq)
{
    Valor v = {0};
    v.frequencia = freq;
    return v;
}

/* Colisao intencional: com m=10 e p=31, h(s) = (soma dos codigos) mod 10 para
 * palavras de 2 letras (31 = 1 mod 10). Todas abaixo caem no bucket 5. */
static const char *COLIDEM[4] = { "ab", "ba", "cj", "di" };

static void t1_insercao_busca(void)
{
    TabelaHash *t = ht_criar(11);
    Valor v = val(3);
    HtStatus st = ht_inserir(t, "casa", &v);
    Valor *r = ht_buscar(t, "casa");
    char obt[160];
    snprintf(obt, sizeof obt, "status=%d, buscar=%s, freq=%d, n=%zu", st, r ? "achou" : "NULL",
             r ? r->frequencia : -1, t->n);
    caso("1. Insercao e busca de chave existente", "inserir(\"casa\", freq=3); buscar(\"casa\")",
         "status=0, achou, freq=3, n=1", obt, st == HT_OK && r && r->frequencia == 3 && t->n == 1);
    ht_destruir(t);
}

static void t2_busca_inexistente(void)
{
    TabelaHash *t = ht_criar(11);
    Valor v = val(1);
    ht_inserir(t, "casa", &v);
    Valor *r = ht_buscar(t, "carro");
    caso("2. Busca de chave inexistente", "tabela {casa}; buscar(\"carro\")", "NULL",
         r ? "achou (errado)" : "NULL", r == NULL);
    ht_destruir(t);
}

static void t3_duplicada(void)
{
    TabelaHash *t = ht_criar(11);
    Valor a = val(3), b = val(9);
    ht_inserir(t, "casa", &a);
    HtStatus st = ht_inserir(t, "casa", &b);
    Valor *r = ht_buscar(t, "casa");
    char obt[160];
    snprintf(obt, sizeof obt, "status=%d, freq=%d, n=%zu", st, r ? r->frequencia : -1, t->n);
    caso("3. Insercao duplicada", "inserir(\"casa\",3); inserir(\"casa\",9)",
         "status=1 (HT_DUPLICADA), freq=3 (original preservado), n=1", obt,
         st == HT_DUPLICADA && r && r->frequencia == 3 && t->n == 1 && t->colisoes == 0);
    ht_destruir(t);
}

static void t4_t5_colisao_remocao(void)
{
    TabelaHash *t = ht_criar(10);
    Valor v = val(1);
    int mesmo = 1;
    for (int i = 0; i < 4; i++) {
        if (ht_hash(COLIDEM[i], 10) != 5) mesmo = 0;
        ht_inserir(t, COLIDEM[i], &v);
    }
    HtEstatisticas e = ht_estatisticas(t);
    char obt[200];
    snprintf(obt, sizeof obt, "h=5 para todas: %s; colisoes=%llu; ocupados=%zu; maior_cadeia=%zu",
             mesmo ? "sim" : "nao", t->colisoes, e.buckets_ocupados, e.maior_cadeia);
    caso("4. Chaves com colisao demonstrada", "inserir ab, ba, cj, di com m=10 (mesmo bucket)",
         "h=5 para todas: sim; colisoes=3; ocupados=1; maior_cadeia=4", obt,
         mesmo && t->colisoes == 3 && e.buckets_ocupados == 1 && e.maior_cadeia == 4);
    puts("    exibirEstrutura() apos as insercoes:");
    ht_exibir_estrutura(t, stdout, 1, 0);
    puts("");

    /* Remove meio ("ba"), cabeca ("di" = ultima inserida) e cauda ("ab"). */
    int ok = 1;
    char passos[200] = "";
    const char *ordem[3] = { "ba", "di", "ab" };
    for (int i = 0; i < 3; i++) {
        HtStatus st = ht_remover(t, ordem[i]);
        int ausente = ht_buscar(t, ordem[i]) == NULL;
        int restantes = 1;
        for (int j = 0; j < 4; j++) {
            int removida = 0;
            for (int k = 0; k <= i; k++) if (!strcmp(COLIDEM[j], ordem[k])) removida = 1;
            if (!removida && !ht_buscar(t, COLIDEM[j])) restantes = 0;
        }
        int integra = ht_verificar_integridade(t);
        if (st != HT_OK || !ausente || !restantes || !integra) ok = 0;
        char tmp[40];
        snprintf(tmp, sizeof tmp, "%s%s:%s", i ? " | " : "", ordem[i], (st == HT_OK && ausente && restantes && integra) ? "ok" : "FALHA");
        strcat(passos, tmp);
    }
    Valor *cj = ht_buscar(t, "cj");
    ok = ok && cj && t->n == 1;
    char obt2[260];
    snprintf(obt2, sizeof obt2, "%s; sobrou cj=%s; n=%zu", passos, cj ? "sim" : "nao", t->n);
    caso("5. Remocao em colisao + busca das restantes",
         "remover ba (meio), di (cabeca), ab (cauda); apos cada uma buscar as demais",
         "ba:ok | di:ok | ab:ok; sobrou cj=sim; n=1", obt2, ok);
    puts("    exibirEstrutura() apos as remocoes:");
    ht_exibir_estrutura(t, stdout, 1, 0);
    puts("");
    HtStatus again = ht_remover(t, "ba");
    caso("5b. Remover chave ja removida", "remover(\"ba\") de novo", "status=2 (HT_NAO_ENCONTRADA)",
         again == HT_NAO_ENCONTRADA ? "status=2" : "outro", again == HT_NAO_ENCONTRADA);
    ht_destruir(t);
}

static void t6_atualizacao(void)
{
    TabelaHash *t = ht_criar(11);
    Valor a = val(1);
    ht_inserir(t, "casa", &a);

    Valor novo = val(42);
    valor_add_ocorrencia(&novo, 1);
    valor_add_ocorrencia(&novo, 7);
    HtStatus st = ht_atualizar(t, "casa", &novo);
    HtStatus st2 = ht_atualizar(t, "carro", &novo);
    valor_liberar(&novo);

    Valor *r = ht_buscar(t, "casa");
    char obt[200];
    snprintf(obt, sizeof obt, "status=%d, freq=%d, linhas=%zu, n=%zu, atualizar(inexistente)=%d",
             st, r ? r->frequencia : -1, r ? r->n_ocorrencias : 0, t->n, st2);
    caso("6. Atualizacao sem alterar a chave", "atualizar(\"casa\", freq=42, linhas=[1,7]); atualizar(\"carro\", ...)",
         "status=0, freq=42, linhas=2, n=1, atualizar(inexistente)=2", obt,
         st == HT_OK && r && r->frequencia == 42 && r->n_ocorrencias == 2 && t->n == 1 && st2 == HT_NAO_ENCONTRADA);
    ht_destruir(t);
}

static void t7_limites(void)
{
    Valor v = val(1);
    TabelaHash *t = ht_criar(7);
    HtEstatisticas e = ht_estatisticas(t);
    int vazia = t->n == 0 && ht_buscar(t, "x") == NULL && ht_remover(t, "x") == HT_NAO_ENCONTRADA &&
                ht_atualizar(t, "x", &v) == HT_NAO_ENCONTRADA && e.buckets_ocupados == 0 && e.maior_cadeia == 0;
    puts("    exibirEstrutura() da tabela vazia:");
    ht_exibir_estrutura(t, stdout, 1, 0);
    puts("");
    caso("7a. Estrutura vazia", "buscar/remover/atualizar em tabela vazia (m=7)",
         "NULL / HT_NAO_ENCONTRADA / HT_NAO_ENCONTRADA, n=0", vazia ? "conforme" : "divergiu", vazia);

    int invalidas = ht_inserir(t, "", &v) == HT_ERRO && ht_inserir(t, NULL, &v) == HT_ERRO &&
                    ht_inserir(NULL, "a", &v) == HT_ERRO && ht_criar(0) == NULL && ht_buscar(t, NULL) == NULL;
    caso("7b. Entradas invalidas", "chave \"\" ou NULL, tabela NULL, ht_criar(0)",
         "HT_ERRO / HT_ERRO / HT_ERRO / NULL / NULL", invalidas ? "conforme" : "divergiu", invalidas);
    ht_destruir(t);

    /* m = 1: todas as chaves colidem no unico bucket */
    t = ht_criar(1);
    const char *ch[3] = { "um", "dois", "tres" };
    int ok = 1;
    for (int i = 0; i < 3; i++) ok &= ht_inserir(t, ch[i], &v) == HT_OK;
    ok &= t->colisoes == 2 && ht_estatisticas(t).maior_cadeia == 3;
    for (int i = 0; i < 3; i++) ok &= ht_buscar(t, ch[i]) != NULL;
    for (int i = 0; i < 3; i++) ok &= ht_remover(t, ch[i]) == HT_OK && ht_verificar_integridade(t);
    ok &= t->n == 0;
    caso("7c. Capacidade minima (m=1)", "inserir 3 chaves, buscar todas, remover todas",
         "colisoes=2, cadeia=3, depois n=0 e integridade ok", ok ? "conforme" : "divergiu", ok);
    ht_destruir(t);

    /* hash sempre dentro de [0, m-1] */
    size_t ms[] = { 1, 2, 7, 10, 101, 10007 };
    const char *ks[] = { "a", "zz", "ab", "coracao", "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz" };
    int faixa = 1;
    for (size_t i = 0; i < sizeof ms / sizeof *ms; i++)
        for (size_t j = 0; j < sizeof ks / sizeof *ks; j++)
            if (ht_hash(ks[j], ms[i]) >= ms[i]) faixa = 0;
    caso("7d. Indice sempre valido", "ht_hash em 6 capacidades x 5 chaves (incl. chave longa)",
         "0 <= h < m em todos os casos", faixa ? "conforme" : "divergiu", faixa);
}

static void t8_normalizacao_e_indice(void)
{
    char a[PALAVRA_MAX + 1], b[PALAVRA_MAX + 1], c[PALAVRA_MAX + 1];
    normalizar_palavra("Coração", a, sizeof a);
    normalizar_palavra("CORAÇÃO!", b, sizeof b);
    normalizar_palavra("coracao", c, sizeof c);
    char obt[256];
    snprintf(obt, sizeof obt, "\"%s\", \"%s\", \"%s\"", a, b, c);
    caso("8a. Normalizacao (caixa, acentos, pontuacao)", "\"Coração\", \"CORAÇÃO!\", \"coracao\"",
         "\"coracao\", \"coracao\", \"coracao\"", obt, !strcmp(a, "coracao") && !strcmp(b, a) && !strcmp(c, a));

    TabelaHash *t = ht_criar(13);
    size_t lidas = indexar_texto(t, "O gato viu o gato.\nO Gato dormiu, bem-estar!");
    Valor *g = ht_buscar(t, "gato");
    Valor *o = ht_buscar(t, "o");
    int ok = lidas == 10 && t->n == 6 && g && g->frequencia == 3 && g->n_ocorrencias == 3 &&
             g->ocorrencias[0] == 1 && g->ocorrencias[1] == 1 && g->ocorrencias[2] == 2 &&
             o && o->frequencia == 3 && ht_buscar(t, "bem") && ht_buscar(t, "estar");
    snprintf(obt, sizeof obt, "palavras=%zu, distintas=%zu, gato.freq=%d, o.freq=%d", lidas, t->n,
             g ? g->frequencia : -1, o ? o->frequencia : -1);
    caso("8b. Indexacao de texto (frequencia e linhas)", "\"O gato viu o gato.\\nO Gato dormiu, bem-estar!\"",
         "palavras=10, distintas=6, gato.freq=3 (linhas 1,1,2), o.freq=3, 'bem' e 'estar' separadas", obt, ok);
    ht_destruir(t);
}

static void t9_estresse(void)
{
    TabelaHash *t = ht_criar(101);          /* alpha ~ 19.8: cadeias longas */
    Valor v = val(1);
    char k[16];
    int ok = 1;
    for (int i = 0; i < 2000; i++) { snprintf(k, sizeof k, "w%05d", i); ok &= ht_inserir(t, k, &v) == HT_OK; }
    ok &= t->n == 2000 && ht_verificar_integridade(t);
    for (int i = 1; i < 2000; i += 2) { snprintf(k, sizeof k, "w%05d", i); ok &= ht_remover(t, k) == HT_OK; }
    for (int i = 0; i < 2000; i++) {
        snprintf(k, sizeof k, "w%05d", i);
        ok &= (ht_buscar(t, k) != NULL) == (i % 2 == 0);
    }
    ok &= t->n == 1000 && ht_verificar_integridade(t);
    caso("9. Estresse: 2000 chaves em m=101, remove as impares",
         "inserir w00000..w01999; remover as de indice impar; buscar todas",
         "n=1000, pares presentes, impares ausentes, integridade ok", ok ? "conforme" : "divergiu", ok);
    ht_destruir(t);
}

int main(void)
{
    puts("=== Testes de correcao | config 220: E2.2 + H6 + C1 + A4 ===");
    puts("(Caso 8 do enunciado - redimensionamento - e exclusivo de E1.2; nao se aplica a E2.2.)\n");
    t1_insercao_busca();
    t2_busca_inexistente();
    t3_duplicada();
    t4_t5_colisao_remocao();
    t6_atualizacao();
    t7_limites();
    t8_normalizacao_e_indice();
    t9_estresse();
    printf("Resumo: %d/%d casos aprovados\n", aprovados, total);
    return aprovados == total ? 0 : 1;
}
