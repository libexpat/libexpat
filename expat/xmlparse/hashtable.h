
#include <stddef.h>

typedef struct {
  const char *name;
} NAMED;

typedef struct {
  NAMED **v;
  size_t size;
  size_t used;
  size_t usedLim;
} HASH_TABLE;

NAMED *lookup(HASH_TABLE *table, const char *name, size_t createSize);
void hashTableInit(HASH_TABLE *);
void hashTableDestroy(HASH_TABLE *);

