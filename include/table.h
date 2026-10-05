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
int table_insert(Table *table, int id, const char *name, int price, int stock);

Record *table_select(Table *table, int id);
int table_update(Table *table, int id, int new_price, int new_stock);
int table_delete(Table *table, int id);


#endif


