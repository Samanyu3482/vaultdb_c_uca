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
    int cap = (initial_capacity > 0) ? initial_capacity : 1;
    table -> records = malloc(cap * sizeof(Record));
    if(table -> records == NULL) {
        free(table->name);
        free(table);
        return NULL;
    }
    table -> record_count = 0;
    table -> capacity = cap;

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


Record *table_select(Table *table, int id) {
    if (table == NULL) return NULL;

    for (int i = 0; i < table->record_count; i++) {
        if (table->records[i].id == id) {
            return &table->records[i];
        }
    }

    return NULL;
}


int table_update(Table *table, int id, int new_price, int new_stock) {
    Record *rec = table_select(table, id);
    if (rec == NULL) return -1;

    rec->price = new_price;
    rec->stock = new_stock;
    return 0;
}

int table_delete(Table *table, int id) {
    if (table == NULL) return -1;

    for (int i = 0; i < table->record_count; i++) {
        if (table->records[i].id == id) {
            free(table->records[i].name);

            for (int j = i; j < table->record_count - 1; j++) {
                table->records[j] = table->records[j + 1];
            }

            table->record_count--;
            return 0;
        }
    }

    return -1;
}



