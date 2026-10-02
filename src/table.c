#include"table.h"
#include<stdlib.h>
#include<string.h>


Table *table_create(const char* name, int initial_capacity) {
    Table *table = malloc(sizeof(Table));
    if(table == NULL) {
        return NULL;
    }
    table -> name = malloc(strlen(name) + 1);
    strcpy(table -> name, name);
    table -> records = malloc(initial_capacity * sizeof(Record));
    if(table -> records == NULL) {
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


