#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdio.h>

#define HT_BASE_POLINOMIAL 31u

typedef enum {
    HT_OK = 0,
    HT_DUPLICADA = 1,        
    HT_NAO_ENCONTRADA = 2,   
    HT_ERRO = -1             
} HtStatus;

typedef struct {
    int frequencia;
    int *ocorrencias;        
    size_t n_ocorrencias;
    size_t cap_ocorrencias;
} Valor;

typedef struct No {
    char *chave;
    Valor valor;
    struct No *ant;
    struct No *prox;
} No;

typedef struct {
    No **buckets;            
    size_t m;                
    size_t n;                
    unsigned long long colisoes;     
    unsigned long long comparacoes;  
} TabelaHash;

typedef struct {
    size_t n, m;
    double alpha;                   
    size_t buckets_ocupados;
    size_t maior_cadeia;
    double media_cadeia_ocupados;
    unsigned long long colisoes;
    unsigned long long comparacoes;
} HtEstatisticas;

void valor_liberar(Valor *v);
int  valor_add_ocorrencia(Valor *v, int linha);      
int  valor_copiar(Valor *dst, const Valor *src);     

TabelaHash *ht_criar(size_t m);                      
void        ht_destruir(TabelaHash *t);
size_t      ht_hash(const char *chave, size_t m);    

HtStatus ht_inserir(TabelaHash *t, const char *chave, const Valor *valor); 
Valor   *ht_buscar(TabelaHash *t, const char *chave);                    
HtStatus ht_atualizar(TabelaHash *t, const char *chave, const Valor *valor);
HtStatus ht_remover(TabelaHash *t, const char *chave);


void           ht_exibir_estrutura(const TabelaHash *t, FILE *out,
                                   int apenas_ocupados, size_t max_buckets);
HtEstatisticas ht_estatisticas(const TabelaHash *t);
void           ht_zerar_contadores(TabelaHash *t);
int            ht_verificar_integridade(const TabelaHash *t); 
void           ht_para_cada(const TabelaHash *t,
                            void (*fn)(const char *chave, const Valor *v, void *ctx),
                            void *ctx);

#endif