#include <stdio.h>
#include"record.h"
#include"table.h"

int main(void) {
    printf("VaultDB starting...\n");
    
    Table *products = table_create("products", 4);
    if(products == NULL) {
        printf("Table not created\n");
        return 1;
    }
    printf("%s\n", products->name);
    printf("%d\n", products->record_count);
    printf("%d\n", products->capacity);
    table_destroy(products);
    printf("Table destroyed successfully\n");
    return 0;
}
