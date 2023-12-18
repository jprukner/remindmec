#include "hash_map.c"
#include <assert.h>
#include <string.h>

int main() {
  enum lookup_error error;

  struct hash_map *map = hash_map_new(31);
  hash_map_add(map, "new", 42);
  int value = hash_map_get(map, "new", &error);
  assert(error == NO_ERROR);
  assert(value == 42);

  hash_map_add(map, "key", 51);
  value = hash_map_get(map, "key", &error);
  assert(error == NO_ERROR);
  assert(value == 51);

  hash_map_add(map, "kez", 50);
  value = hash_map_get(map, "kez", &error);
  assert(error == NO_ERROR);
  assert(value == 50);

  hash_map_add(map, "kez8000asdff", 82);
  value = hash_map_get(map, "kez8000asdff", &error);
  assert(error == NO_ERROR);
  assert(value == 82);

  hash_map_add(map, "kez8da00asdff", 500);
  value = hash_map_get(map, "kez8da00asdff", &error);
  assert(error == NO_ERROR);
  assert(value == 500);

  value = hash_map_get(map, "NOKEYLIKETHIS", &error);
  assert(error == ERR_KEY_NOT_FOUND);
  assert(value == 0);
}