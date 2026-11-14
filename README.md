# Tabela Hash própria — Configuração 220

**E2.2 · H6 · C1 · A4**

| Código | Significado |
|---|---|
| **E2.2** | Endereçamento fechado: vetor de buckets, cada um com uma **lista duplamente encadeada** |
| **H6** | Função hash **polinomial** (Horner, base 31) |
| **C1** | **Encadeamento separado** |
| **A4** | **Índice de palavras** (chave: palavra normalizada; valor: frequência e linhas de ocorrência) |

Implementação em C11, sem `dict`/`HashMap`/equivalentes: a tabela é um vetor de ponteiros para nós alocados com `malloc`.

## Estrutura do repositório

```
src/hash_table.{h,c}   tabela hash (E2.2 + H6 + C1)
src/palavras.{h,c}     aplicação A4: normalização, tokenização e indexação
src/main.c             programa interativo (índice de palavras)
src/test_hash.c        testes de correção obrigatórios
src/bench.c            análise experimental
data/exemplo.txt       texto de exemplo
resultados.csv         saída do benchmark
Makefile
```

## Como compilar e executar

```bash
make                  # compila indice, test_hash e bench
make test             # testes de correção (entradas, esperado, obtido, situação)
make asan             # testes com AddressSanitizer + UBSan
make run              # abre o índice com data/exemplo.txt (m=101)
make experimentos     # 3 tamanhos x 3 capacidades x 5 repetições -> resultados.csv
./indice -m 1009 meu_texto.txt     # capacidade e arquivos à escolha
```

Comandos do programa: `carregar`, `inserir`, `indexar`, `buscar`, `atualizar`, `remover`, `listar`, `estrutura`, `stats`, `ajuda`, `sair`.

## 1. Projeto

### Desenho da tabela (E2.2)

```
buckets[0] -> NULL
buckets[1] -> NULL <-> ["gato"] <-> ["casa"] <-> NULL
buckets[2] -> NULL <-> ["lista"] <-> NULL
   ...
buckets[m-1] -> NULL
```

Cada nó guarda `chave`, `valor`, `ant` e `prox`. O ponteiro `ant` permite desvincular um nó em O(1) depois de localizado, sem rastrear o predecessor, e a cabeça tem `ant == NULL`.

### Função hash (H6) e cálculo do índice

Para uma palavra `s` de comprimento `L`:

```
h(s) = (s[0]·p^(L-1) + s[1]·p^(L-2) + ... + s[L-1]) mod m,   p = 31
```

Calculada por Horner: `h = (h·31 + s[i]) mod m`. A redução a cada passo mantém `h < m`, sem risco de estouro. O resultado está sempre em `[0, m-1]` (testado no caso 7d). `m` primo é recomendado; o benchmark usa primos.

### Conversão de chave e normalização (A4)

A chave é uma palavra normalizada, e o hash opera sobre os bytes dessa palavra. Regras (`palavras.c`):

1. letras maiúsculas viram minúsculas;
2. acentos do Latin-1 em UTF-8 são removidos (`Coração`, `CORAÇÃO`, `coracao` → `coracao`);
3. palavra = sequência máxima de letras ASCII e dígitos; qualquer outro símbolo (espaço, pontuação, hífen, apóstrofo) é separador (`bem-estar` → `bem`, `estar`);
4. palavras com mais de 63 caracteres são truncadas.

Para chaves digitadas em `buscar`/`remover`/etc., usa-se a primeira palavra da entrada.

### Políticas

| Situação | Política |
|---|---|
| **Colisão** | Chave nova cujo bucket já está ocupado: vai para a cabeça da lista e `colisoes++`. Atualizar chave existente não conta. |
| **Duplicata** | `ht_inserir` retorna `HT_DUPLICADA` e preserva o registro original. Para somar uma ocorrência, a aplicação usa `indexar_palavra`. |
| **Busca sem resultado** | `ht_buscar` retorna `NULL`; `ht_remover`/`ht_atualizar` retornam `HT_NAO_ENCONTRADA`. |
| **Argumentos inválidos** | Chave `NULL` ou vazia, tabela `NULL` ou `m = 0` resultam em `HT_ERRO` / `NULL`. |
| **Valores** | `ht_inserir` e `ht_atualizar` copiam o valor (cópia profunda); o chamador mantém a posse do original. |

### Parâmetros

- Capacidade inicial: `m = 1009` por padrão em `./indice` (opção `-m`); no benchmark, `m ≈ n/2`, `n` e `2n` (primos).
- Base polinomial: `p = 31`.
- Sem rehashing: C1 degrada de forma suave com α > 1, e o rehashing é exigido apenas para E1.2.

### Valor armazenado (A4)

`Valor { frequencia, ocorrencias[], n_ocorrencias }`, em que `ocorrencias` guarda o número da linha de cada ocorrência (vetor dinâmico com crescimento por dobra).

## 2. Implementação

API mínima exigida:

| Requisito | Função |
|---|---|
| `inserir(chave, valor)` | `ht_inserir` |
| `buscar(chave)` | `ht_buscar` |
| `remover(chave)` | `ht_remover` |
| `atualizar(chave, valor)` | `ht_atualizar` |
| `exibirEstrutura()` | `ht_exibir_estrutura` |
| contadores de colisões e ocupação | `TabelaHash.colisoes`, `.comparacoes`, `ht_estatisticas` |
| listar registros | `ht_para_cada` (o programa ordena em `listar`) |

Há também `ht_verificar_integridade`, que confere os elos `ant`/`prox`, o bucket correto de cada nó e a contagem `n`.

## 3. Testes de correção

`make test` imprime, para cada caso, entradas, resultado esperado, resultado obtido e situação. O caso 8 do enunciado (redimensionamento) é exclusivo de E1.2 e não se aplica a esta configuração.

| # | Caso |
|---|---|
| 1 | Inserção e busca de chave existente |
| 2 | Busca de chave inexistente |
| 3 | Inserção duplicada |
| 4 | **Colisão intencional**: `ab`, `ba`, `cj`, `di` com `m=10` caem todas no bucket 5 |
| 5 | **Remoção em colisão**: remove meio, cabeça e cauda da cadeia; busca as restantes após cada remoção |
| 6 | Atualização sem alterar a chave |
| 7 | Estrutura vazia, entradas inválidas, `m=1`, índice sempre em `[0, m-1]` |
| 8 | Normalização e indexação de texto |
| 9 | Estresse: 2000 chaves em `m=101`, remoção das ímpares |

Por que as colisões do caso 4 são previsíveis: com `p = 31 ≡ 1 (mod 10)`, o hash de palavras de 2 letras é a soma dos códigos mod 10 (`97+98 = 98+97 = 99+106 = 100+105 = 195`).

## 4. Análise experimental

Metodologia (`bench.c`):

- n = 100, 1.000 e 10.000 palavras de 7 letras, e para cada n três capacidades (`m ≈ n/2`, `n`, `2n`, primos);
- **5 repetições** por cenário, com sementes diferentes; os valores abaixo são médias;
- conjuntos presente e ausente disjuntos, mesmo n e mesma regra de medição em todas as repetições;
- colisão = bucket inicial já ocupado por uma chave nova; `cmp` = comparações de chaves por busca;
- tempo de inserção = tempo total para inserir as n palavras; tempo de busca = tempo total para buscar as n palavras (média de várias passadas para n pequeno);
- relógio `CLOCK_MONOTONIC`, compilação com `-O2`.

Resultados de uma execução (os tempos dependem da máquina; rode `make experimentos` no seu ambiente e substitua):

| Registros | Cap. | α | Colisões | Ins. (ms) | Busca ok (ms) | Busca aus. (ms) | cmp/ok | cmp/aus | Maior cadeia |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 100 | 53 | 1,89 | 56,2 | 0,0132 | 0,0039 | 0,0039 | 2,03 | 1,87 | 5,6 |
| 100 | 101 | 0,99 | 35,2 | 0,0095 | 0,0037 | 0,0034 | 1,48 | 1,01 | 4,8 |
| 100 | 211 | 0,47 | 23,0 | 0,0140 | 0,0035 | 0,0029 | 1,29 | 0,46 | 3,8 |
| 1.000 | 503 | 1,99 | 569,6 | 0,1354 | 0,0521 | 0,0502 | 1,99 | 1,97 | 7,0 |
| 1.000 | 1009 | 0,99 | 366,6 | 0,1228 | 0,0452 | 0,0437 | 1,49 | 0,99 | 5,8 |
| 1.000 | 2003 | 0,50 | 208,0 | 0,1246 | 0,0416 | 0,0376 | 1,24 | 0,51 | 4,2 |
| 10.000 | 5003 | 2,00 | 5672,8 | 1,4428 | 0,8956 | 0,9434 | 2,00 | 2,01 | 9,2 |
| 10.000 | 10007 | 1,00 | 3663,8 | 0,9945 | 0,6598 | 0,6905 | 1,50 | 1,00 | 7,2 |
| 10.000 | 20011 | 0,50 | 2142,8 | 0,6473 | 0,4791 | 0,4776 | 1,25 | 0,50 | 5,2 |

### Interpretação

- **α e comparações.** Para cada n, as comparações por busca seguem o modelo teórico de encadeamento com hash uniforme: busca com sucesso ≈ 1 + α/2 e busca sem sucesso ≈ α (por exemplo, α = 2,00 → 2,00 e 2,01; α = 0,50 → 1,25 e 0,50). Isso indica que H6 com `p = 31` e `m` primo distribui bem estas chaves.
- **α e colisões.** O número de colisões cresce com α, e as cadeias mais longas também (maior cadeia 3,8 → 9,2 ao subir α e n). Como C1 aceita α > 1, a tabela continua correta com α = 2; o custo é só o comprimento das cadeias.
- **Busca sem sucesso** precisa percorrer a cadeia inteira (≈ α comparações), mas custa menos comparações que a busca com sucesso quando α < 1, porque muitos buckets estão vazios.
- **Tempo.** Cresce com n de forma pouco mais que linear por operação. Na tabela de 10.000, o tempo por busca sobe além do esperado pelo número de comparações, o que é compatível com o custo de acesso à memória (nós alocados separadamente, pior localidade de cache), efeito que a contagem de comparações não captura.

## 5. Decisões de projeto

- **Lista dupla (E2.2):** a remoção não precisa guardar o predecessor durante a busca; o custo é um ponteiro extra por nó.
- **Inserção na cabeça:** O(1) depois da verificação de duplicata, que já percorre a cadeia.
- **Contagem de colisões:** feita só para chave nova, como pede a definição operacional; duplicatas e atualizações não contam.
- **Cópia profunda do valor:** evita *aliasing* entre o chamador e a tabela, simplifica a posse da memória e foi verificada com AddressSanitizer.
- **Normalização fora da tabela:** a tabela hasheia bytes quaisquer; as regras de A4 ficam em `palavras.c`, o que mantém `hash_table` reutilizável.

## 6. Limitações

- A normalização de acentos cobre o Latin-1 (português, espanhol, francês etc.); outros alfabetos viram separadores.
- Não há redimensionamento automático (não exigido por E2.2).
- Os tempos do benchmark foram medidos em um único ambiente.
