#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED 0x12345678

typedef struct tMunicipios {

  char cod_ibge[8];
  char nome[100];
  float latitude;
  float longitude;
  int capital;
  int cod_uf;
  int cod_siafi;
  int ddd;
  char fuso_hor[100];

} tMunicipios;

char *get_key(void *reg) { return (*((tMunicipios *)reg)).cod_ibge; }

typedef struct {
  uintptr_t *table;
  int size;
  int max;
  uintptr_t deleted;
  char *(*get_key)(void *);
} thash;

uint32_t hashf(const char *str, uint32_t h) {
  /* One-byte-at-a-time Murmur hash
  Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
  for (; *str; ++str) {
    h ^= *str;
    h *= 0x5bd1e995;
    h ^= h >> 15;
  }
  return h;
}

uint32_t hashf2(const char *str, uint32_t h) {
  /* Another hash function for double hashing */
  for (; *str; ++str) {
    h ^= *str;
    h *= 0x9e3779b1;
  }
  return h;
}

int hash_insere(thash *h, void *bucket) {
  uint32_t hash = hashf(h->get_key(bucket), SEED);
  uint32_t hash2 = hashf2(h->get_key(bucket), SEED);

  int pos = hash % h->max;
  int step = hash2 % (h->max - 1) + 1;

  if (h->max == (h->size + 1)) {
    free(bucket);
    return EXIT_FAILURE;
  } else {
    while (h->table[pos] != 0) {
      if (h->table[pos] == h->deleted)
        break;
      pos = (pos + step) % h->max;
    }
    h->table[pos] = (uintptr_t)bucket;
    h->size += 1;
  }
  return EXIT_SUCCESS;
}

int hash_constroi(thash *h, int nbuckets, char *(*get_key)(void *)) {
  h->table = calloc(sizeof(uintptr_t), nbuckets + 1);
  if (h->table == NULL) {
    return EXIT_FAILURE;
  }
  h->max = nbuckets + 1;
  h->size = 0;
  h->deleted = (uintptr_t) & (h->size);
  h->get_key = get_key;
  return EXIT_SUCCESS;
}

void *hash_busca(thash h, const char *key) {
  int pos = hashf(key, SEED) % h.max;
  int step = hashf2(key, SEED) % (h.max - 1) + 1;

  while (h.table[pos] != 0) {
    if (strcmp(h.get_key((void *)h.table[pos]), key) == 0)
      return (void *)h.table[pos];
    else
      pos = (pos + step) % h.max;
  }
  return NULL;
}

int hash_remove(thash *h, const char *key) {
  int pos = hashf(key, SEED) % h->max;
  int step = hashf2(key, SEED) % (h->max - 1) + 1;

  while (h->table[pos] != 0) {
    if (strcmp(h->get_key((void *)h->table[pos]), key) == 0) {
      free((void *)h->table[pos]);
      h->table[pos] = h->deleted;
      h->size -= 1;
      return EXIT_SUCCESS;
    } else {
      pos = (pos + step) % h->max;
    }
  }
  return EXIT_FAILURE;
}

void hash_apaga(thash *h) {
  int pos;
  for (pos = 0; pos < h->max; pos++) {
    if (h->table[pos] != 0) {
      if (h->table[pos] != h->deleted) {
        free((void *)h->table[pos]);
      }
    }
  }
  free(h->table);
}

int main(int argc, char *argv[]) {

  FILE *arquivo1;

  arquivo1 = fopen("municipios.txt", "r");
  if (arquivo1 == NULL) {
    printf("Erro ao abrir o arquivo de Municipios!");
    return 0;
  }

  thash Hash;
  int nbuckets = 7000;

  if (hash_constroi(&Hash, nbuckets, get_key) != EXIT_SUCCESS) {
    printf("Erro ao criar a tabela hash\n");
    return 0;
  }

  while (!feof(arquivo1)) {
    tMunicipios *novo = malloc(sizeof(tMunicipios));
    int x = fscanf(arquivo1, " %[^,], %[^,], %f, %f, %d, %d, %d, %d, %[^\n]",
                   novo->cod_ibge, novo->nome, &novo->latitude,
                   &novo->longitude, &novo->capital, &novo->cod_uf,
                   &novo->cod_siafi, &novo->ddd, novo->fuso_hor);

    
    if (hash_insere(&Hash, novo) != EXIT_SUCCESS) {
      printf("Erro ao inserir elemento na tabela hash\n");
      return 0;
    }
  }

  const char *key = "2100600";
  tMunicipios *elemento = (tMunicipios *)hash_busca(Hash, key);

  if (elemento != NULL) {
    printf("Elemento encontrado:\n");
    printf("Código IBGE: %s\n", elemento->cod_ibge);
    printf("Nome: %s\n", elemento->nome);
    // Imprima os demais campos que você deseja mostrar
  } else {
    printf("Elemento não encontrado.\n");
  }



  return 0;
}
