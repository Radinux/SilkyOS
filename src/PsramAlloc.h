#ifndef PSRAM_ALLOC_H
#define PSRAM_ALLOC_H

#include <ArduinoJson.h>
#include <esp_heap_caps.h>

// Fournisseur de mémoire pour ArduinoJson : tout est pris en PSRAM,
// pour garder la RAM interne (précieuse) libre.
struct AllocateurPsram : ArduinoJson::Allocator {
  void *allocate(size_t n) override           { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM); }
  void  deallocate(void *p) override           { heap_caps_free(p); }
  void *reallocate(void *p, size_t n) override { return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM); }
};

// Une seule instance, partagée par tous les fichiers
inline AllocateurPsram *allocPsram() {
  static AllocateurPsram instance;
  return &instance;
}

#endif