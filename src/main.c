#include <stdio.h>

#include "table.h"

int main(void)
{
    struct Table *products = table_create("products", 4);

    if (products == NULL)
    {
        printf("Failed to create table.\n");
        return 1;
    }

    printf("Table: %s\n", products->name);
    printf("Record count: %d\n", products->record_count);
    printf("Capacity: %d\n", products->capacity);

    table_destroy(products);

    return 0;
}
