#ifndef RECORD_H
#define RECORD_H

typedef struct Record {
  int id;
  char *name;
  int price;
  int stock;
} Record;

Record *record_create(int id, const char *name, int price, int stock);

void record_destroy(Record *record);

#endif
