#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ASSERT(expr) assert(expr)

#define ALLOC(sz) malloc(sz)
#define REALLOC(ptr, sz) realloc(ptr, sz)
#define DEALLOC(ptr) free(ptr)

typedef struct {
  const char *key;
  const char *value;
} KV;

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

KV kv_init(const char *key, const char *value) {
  KV kv;
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

Dict dict_init(size_t len) {
  Dict d = {0};
  Bucket *mem = (Bucket *)ALLOC(len * sizeof(Bucket));
  ASSERT(mem && "alloc failed");
  d.buckets = mem;
  d.len = len;
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

size_t only_for_testing_hash_fn(const char *key) {
  return cstring_sz(key);
}

int dict_add(Dict *d, const char *key, const char *value) {
  size_t idx = only_for_testing_hash_fn(key);
  if (idx >= d->len) {
    size_t new_len = idx * 2; // only for testing
    Bucket *mem = (Bucket *)REALLOC(d->buckets, new_len * sizeof(Bucket));
    ASSERT(mem && "realloc failed");
    d->buckets = mem;
    d->len = new_len;
  }
  Bucket *b = d->buckets + idx;
  if (b->cap == 0) {
    *b = bucket_init(3); // only for testing
  }
  for (size_t i = 0; i < b->sz; ++i) {
    if (cstring_eq(key, b->kvs[i].key)) return 1;
  }
  bucket_add(b, key, value);
  return 0;
}

int dict_contains_key(Dict *d, const char *key) {
  ASSERT(d && d->len > 0 && d->buckets && "dict must be initialized");
  size_t idx = only_for_testing_hash_fn(key);
  if (idx >= d->len) return 0;
  Bucket *b = d->buckets + idx;
  if (b->cap == 0) return 0;
  for (size_t i = 0; i < b->sz; ++i) {
    if (cstring_eq(key, b->kvs[i].key)) return 1;
  }
  return 0;
}

int main() {
  Dict d = dict_init(1);
  dict_add(&d, "abc", "123");
  ASSERT(dict_contains_key(&d, "abc"));
  ASSERT(!dict_contains_key(&d, "def"));
  dict_deinit(&d);
  return 0;
}
