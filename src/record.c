#include "record.h"
#include <stdlib.h>
#include <string.h>

Record *record_create(int id, const char *name, int price, int stock) {
  Record *record = malloc(sizeof(Record));
  if (record == NULL) {
    return NULL;
  }
  record->id = id;
  record->price = price;
  record->stock = stock;
  record->name = (char *)malloc(strlen(name) + 1);
  if (record->name == NULL) {
    free(record);
    return NULL;
  }
  strcpy(record->name, name);
  return record;
}

void record_destroy(Record *record) {
  if (record == NULL) {
    return;
  }
  free(record->name);
  free(record);
}
