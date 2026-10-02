#ifndef TABLE_H
#define TABLE_H
#include"record.h"

typedef struct Table {
    char* name;
    Record* records;
    int record_count;
    int capacity;
} Table;

Table *table_create(const char* name, int initial_capacity);

void table_destroy(Table *table);

#endif


