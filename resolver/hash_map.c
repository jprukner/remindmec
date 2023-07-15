#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct hash_map {
  int *array;
  uint64_t size;
};

struct hash_map *hash_map_new(uint64_t prime_number_size) {
  struct hash_map *map = malloc(sizeof(struct hash_map));
  map->size = prime_number_size;
  map->array = malloc(sizeof(int) * prime_number_size);
  return map;
}

void hash_map_free(struct hash_map *map) {
  free(map->array);
  free(map);
}

#define HASH_BYTES_STEP 8

// hash returns hash value of given value.
static uint64_t hash(const char *key) {
  uint64_t result = 0;
  uint64_t tmp = 0;
  uint64_t i = 0;
  uint64_t length = strlen(key);
  while (i + HASH_BYTES_STEP < length) {
    result += tmp;
    memcpy(&tmp, key + i * HASH_BYTES_STEP, HASH_BYTES_STEP);
    i += HASH_BYTES_STEP;
    tmp = 0;
  }

  if (i < length) {
    memcpy(&tmp, key, length - i);
    result += tmp;
  }
  return result;
}

void hash_map_add(struct hash_map *map, const char *key, int value) {
  uint64_t hashed_key = hash(key);
  uint64_t position = hashed_key % map->size;
  map->array[position] = value;
}

int hash_map_get(struct hash_map *map, const char *key) {
  uint64_t hashed_key = hash(key);
  uint64_t position = hashed_key % map->size;
  return map->array[position];
}