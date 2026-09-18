#include <stdlib.h>
#include <string.h>

#include "record.h"

struct Record *record_create(int id, const char *name, int price, int stock)
{
    struct Record *record = malloc(sizeof(struct Record));

    if (record == NULL)
    {
        return NULL;
    }

    record->id = id;
    record->price = price;
    record->stock = stock;

    record->name = malloc(strlen(name) + 1);

    if (record->name == NULL)
    {
        free(record);
        return NULL;
    }

    strcpy(record->name, name);

    return record;
}

void record_destroy(struct Record *record)
{
    if (record == NULL)
    {
        return;
    }

    free(record->name);
    free(record);
}
