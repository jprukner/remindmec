#include "hash_map.c"
#include <assert.h>
#include <string.h>

int main() {
  struct hash_map *map = hash_map_new(31);
  hash_map_add(map, "new", 42);
  int value = hash_map_get(map, "new");
  assert(value == 42);

  hash_map_add(map, "key", 51);
  value = hash_map_get(map, "key");
  assert(value == 51);

  hash_map_add(map, "kez", 50);
  value = hash_map_get(map, "kez");
  assert(value == 50);

  hash_map_add(map, "kez8000asdff", 82);
  value = hash_map_get(map, "kez8000asdff");
  assert(value == 82);

  hash_map_add(map, "kez8da00asdff", 500);
  value = hash_map_get(map, "kez8da00asdff");
  assert(value == 500);
}