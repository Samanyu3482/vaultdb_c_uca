#ifndef TABLE_H
#define TABLE_H

#include "record.h"

struct Table {
    char *name;
    struct Record *records;
    int record_count;
    int capacity;
};

struct Table *table_create(const char *name, int capacity);
void table_destroy(struct Table *table);

#endif
