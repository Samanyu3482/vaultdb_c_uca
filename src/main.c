#include <stdio.h>
#include "table.h"
#include "storage.h"

int main(void) {
    Table *loaded = storage_load_table("data/test.db");
    
    if (loaded == NULL) {
        printf("Failed to load table!\n");
        return 1;
    }

    printf("Successfully loaded table '%s' with %d records:\n", loaded->name, loaded->record_count);
    
    for (int i = 0; i < loaded->record_count; i++) {
        Record *r = &loaded->records[i];
        printf(" - [%d] %s (Price: %d, Stock: %d)\n", r->id, r->name, r->price, r->stock);
    }

    table_destroy(loaded);
    return 0;
}
