#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ASSERT(expr) assert(expr)

#define ALLOC(sz) malloc(sz)
#define REALLOC(ptr, sz) realloc(ptr, sz)
#define DEALLOC(ptr) free(ptr)

#define BUCKET_INIT 0x10
#define DICT_INIT 0x100

void cstring_copy(const char *src, char *dst) {
  for (; *src != '\0'; ++src, ++dst) {
    *dst = *src;
  }
  *dst = '\0';
}

size_t cstring_sz(const char *s) {
  size_t sz = 0;
  for (; *s != '\0'; ++s, ++sz) {}
  return sz + 1;
}

int cstring_eq(const char *s1, const char *s2) {
  for (; *s1 != '\0' && *s2 != '\0'; ++s1, ++s2) {
    if (*s1 != *s2) return 0;
  }
  return *s1 == '\0' && *s2 == '\0';
}

typedef struct {
  const char *key;
  const char *value;
} KV;

KV kv_init(const char *key, const char *value) {
  KV kv = {0};
  size_t key_sz = cstring_sz(key);
  size_t value_sz = cstring_sz(value);
  char *mem = (char *)ALLOC(key_sz + value_sz);
  ASSERT(mem && "alloc failed");
  cstring_copy(key, mem);
  cstring_copy(value, mem + key_sz);
  kv.key = mem;
  kv.value = mem + key_sz;
  return kv;
}

void kv_deinit(KV *kv) {
  ASSERT(kv && kv->key && kv->value && "kv must be initialized");
  DEALLOC((void *)kv->key);
  kv->key = NULL;
  kv->value = NULL;
}

typedef struct {
  KV     *kvs;
  size_t  sz;
  size_t  cap;
} Bucket;

Bucket bucket_init(size_t cap) {
  Bucket b = {0};
  KV *mem = (KV *)ALLOC(cap * sizeof(KV));
  ASSERT(mem && "alloc failed");
  b.kvs = mem;
  b.cap = cap;
  return b;
}

void bucket_deinit(Bucket *b) {
  ASSERT(b && b->cap > 0 && b->kvs && "bucket must be initialized");
  for (size_t i = 0; i < b->sz; ++i) {
    kv_deinit(b->kvs + i);
  }
  DEALLOC(b->kvs);
  b->sz = 0;
  b->cap = 0;
}

void bucket_add(Bucket *b, const char *key, const char *value) {
  if (b->sz == b->cap) {
    size_t new_cap = b->cap * 2;
    KV *mem = (KV *)REALLOC(b->kvs, new_cap * sizeof(KV));
    ASSERT(mem && "realloc failed");
    b->kvs = mem;
    b->cap = new_cap;
  }
  b->kvs[b->sz++] = kv_init(key, value);
}

typedef struct {
  Bucket *buckets;
  size_t  len;
} Dict;

Dict dict_init() {
  Dict d = {0};
  Bucket *mem = (Bucket *)ALLOC(DICT_INIT * sizeof(Bucket));
  ASSERT(mem && "alloc failed");
  d.buckets = mem;
  d.len = DICT_INIT;
  return d;
}

void dict_deinit(Dict *d) {
  ASSERT(d && d->len > 0 && d->buckets && "dict must be initialized");
  for (size_t i = 0; i < d->len; ++i) {
    Bucket *b = d->buckets + i;
    if (b->cap > 0) {
      bucket_deinit(b);
    }
  }
  DEALLOC(d->buckets);
  d->buckets = NULL;
  d->len = 0;
}

// djb2 - http://www.cse.yorku.ca/~oz/hash.html
size_t hash_fn(const char *key) {
  size_t hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

int dict_add(Dict *d, const char *key, const char *value) {
  size_t idx = hash_fn(key) % d->len;
  // resizing: todo
  /*if (should_resize) {
    size_t new_len = dict_new_len(d);
    Bucket *mem = (Bucket *)REALLOC(d->buckets, new_len * sizeof(Bucket));
    ASSERT(mem && "realloc failed");
    d->buckets = mem;
    d->len = new_len;
  }*/
  Bucket *b = d->buckets + idx;
  if (b->cap == 0) {
    *b = bucket_init(BUCKET_INIT);
  }
  for (size_t i = 0; i < b->sz; ++i) {
    if (cstring_eq(key, b->kvs[i].key)) return 1;
  }
  bucket_add(b, key, value);
  return 0;
}

const char *dict_get(Dict *d, const char *key) {
  ASSERT(d && d->len > 0 && d->buckets && "dict must be initialized");
  size_t idx = hash_fn(key) % d->len;
  Bucket *b = d->buckets + idx;
  if (b->cap == 0) return NULL;
  for (size_t i = 0; i < b->sz; ++i) {
    KV kv = b->kvs[i];
    if (cstring_eq(key, kv.key)) return kv.value;
  }
  return NULL;
}

int main() {
  Dict d = dict_init();
  dict_add(&d, "abc", "123");
  ASSERT(dict_get(&d, "abc"));
  ASSERT(!dict_get(&d, "def"));
  dict_deinit(&d);
  return 0;
}
