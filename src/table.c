
#include <stdlib.h>
#include <string.h>

#include "table.h"

struct Table *table_create(const char *name, int capacity)
{
    struct Table *table = malloc(sizeof(struct Table));

    if (table == NULL)
    {
        return NULL;
    }

    table->name = malloc(strlen(name) + 1);

    if (table->name == NULL)
    {
        free(table);
        return NULL;
    }

    strcpy(table->name, name);

    table->records = malloc(capacity * sizeof(struct Record));

    if (table->records == NULL)
    {
        free(table->name);
        free(table);
        return NULL;
    }

    table->record_count = 0;
    table->capacity = capacity;

    return table;
}

void table_destroy(struct Table *table)
{
    if (table == NULL)
    {
        return;
    }

    free(table->records);
    free(table->name);
    free(table);
}
