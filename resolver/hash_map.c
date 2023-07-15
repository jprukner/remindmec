#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct hash_map_meta_value {
  const char *key;
  uint32_t value;
};

struct hash_map {
  struct hash_map_meta_value *array;
  uint64_t size;
};

struct hash_map *hash_map_new(uint64_t prime_number_size) {
  struct hash_map *map = malloc(sizeof(struct hash_map));
  map->size = prime_number_size;
  map->array = malloc(sizeof(struct hash_map_meta_value) * prime_number_size);
  for (int i = 0; i < prime_number_size; i++) {
    struct hash_map_meta_value value = {.value = 0, .key = NULL};
    map->array[i] = value;
  }
  return map;
}

void hash_map_free(struct hash_map *map) {
  free(map->array);
  free(map);
}

#define HASH_BYTES_STEP 8

enum lookup_error { NO_ERROR, ERR_KEY_NOT_FOUND };

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

void hash_map_add(struct hash_map *map, const char *key, uint32_t value) {
  uint64_t hashed_key = hash(key);
  uint64_t position = hashed_key % map->size;
  struct hash_map_meta_value stored_value;
  int32_t quadratic_steps = 0;
  stored_value = map->array[position];
  while (stored_value.key != NULL) {
    position += 1 << quadratic_steps;
    position = position % map->size;
    stored_value = map->array[position];
    quadratic_steps++;
  }
  map->array[position] =
      (struct hash_map_meta_value){.key = key, .value = value};
}

uint32_t hash_map_get(struct hash_map *map, const char *key,
                      enum lookup_error *error) {
  uint64_t hashed_key = hash(key);
  uint64_t position = hashed_key % map->size;
  struct hash_map_meta_value stored_value;
  stored_value = map->array[position];
  int32_t quadratic_steps = 0;
  while (1) {
    if (stored_value.key == NULL) {
      *error = ERR_KEY_NOT_FOUND;
      return 0;
    }
    if (strcmp(stored_value.key, key) == 0) {
      break;
    }
    position += 1 << quadratic_steps;
    position = position % map->size;
    stored_value = map->array[position];
    quadratic_steps++;
  }
  *error = NO_ERROR;
  return stored_value.value;
}