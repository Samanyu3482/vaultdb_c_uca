#include"table.h"
#include<stdlib.h>
#include<string.h>


Table *table_create(const char* name, int initial_capacity) {
    Table *table = malloc(sizeof(Table));
    if(table == NULL) {
        return NULL;
    }
    table -> name = malloc(strlen(name) + 1);
    if(table->name == NULL) {
        free(table);
        return NULL;
    }
    strcpy(table -> name, name);
    table -> records = malloc(initial_capacity * sizeof(Record));
    if(table -> records == NULL) {
        free(table->name);
        free(table);
        return NULL;
    }
    table -> record_count = 0;
    table -> capacity = initial_capacity;

    return table;

}



void table_destroy(Table *table) {
    if(table == NULL) {
        return;
    }
    int n = table -> record_count;
    for(int i = 0; i < n; i++) {
        free(table -> records[i].name);
    }
    free(table -> records);
    free(table -> name);
    free(table);

}


int table_insert(Table *table, int id, const char *name, int price, int stock) {
    if (table == NULL || name == NULL) return -1;

    if (table->record_count == table->capacity) {
        int newCapacity = (table->capacity == 0) ? 4 : table->capacity * 2;
        Record *temp = realloc(table->records, newCapacity * sizeof(Record));
        if (temp == NULL) return -1;  
        table->records = temp;
        table->capacity = newCapacity;
    }

    Record *slot = &table->records[table->record_count];

    slot->name = malloc(strlen(name) + 1);
    if (slot->name == NULL) return -1;
    strcpy(slot->name, name);

    slot->id = id;
    slot->price = price;
    slot->stock = stock;

    table->record_count++;
    return 0;
}