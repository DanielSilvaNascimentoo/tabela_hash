#ifndef PALAVRAS_H
#define PALAVRAS_H

#include "hash_table.h"

#define PALAVRA_MAX 63   

size_t proxima_palavra(const char **texto, char *saida, size_t cap);

size_t normalizar_palavra(const char *entrada, char *saida, size_t cap);

HtStatus indexar_palavra(TabelaHash *t, const char *palavra_normalizada, int linha);

size_t indexar_texto(TabelaHash *t, const char *texto);

#endif
