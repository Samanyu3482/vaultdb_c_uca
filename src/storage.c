#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int storage_save_table(Table *table, const char *filename) {
    if (table == NULL || filename == NULL) return -1;

    FILE *file = fopen(filename, "wb");
    if (file == NULL) return -1;

   
    int name_len = strlen(table->name) + 1;
    fwrite(&name_len, sizeof(int), 1, file);
    fwrite(table->name, sizeof(char), name_len, file);
    fwrite(&table->record_count, sizeof(int), 1, file);


    for (int i = 0; i < table->record_count; i++) {
        Record *r = &table->records[i];

        fwrite(&r->id, sizeof(int), 1, file);
        fwrite(&r->price, sizeof(int), 1, file);
        fwrite(&r->stock, sizeof(int), 1, file);

        int rec_name_len = strlen(r->name) + 1;
        fwrite(&rec_name_len, sizeof(int), 1, file);
        fwrite(r->name, sizeof(char), rec_name_len, file);
    }

    fclose(file);
    return 0;
}

static char *read_string(FILE *file) {
    int len;
    if (fread(&len, sizeof(int), 1, file) != 1 || len <= 0) return NULL;

    char *buf = malloc(len);
    if (buf == NULL) return NULL;

    if (fread(buf, sizeof(char), len, file) != (size_t)len) {
        free(buf);
        return NULL;
    }
    return buf;
}

Table *storage_load_table(const char *filename) {
    if (filename == NULL) return NULL;

    FILE *file = fopen(filename, "rb");
    if (file == NULL) return NULL;

  
    char *table_name = read_string(file);
    int record_count;
    if (table_name == NULL || fread(&record_count, sizeof(int), 1, file) != 1 || record_count < 0) {
        free(table_name);
        fclose(file);
        return NULL;
    }

    Table *table = table_create(table_name, record_count);
    free(table_name);   
    if (table == NULL) {
        fclose(file);
        return NULL;
    }

    
    for (int i = 0; i < record_count; i++) {
        int id, price, stock;
        if (fread(&id, sizeof(int), 1, file) != 1 ||
            fread(&price, sizeof(int), 1, file) != 1 ||
            fread(&stock, sizeof(int), 1, file) != 1) {
            goto fail;
        }

        char *rec_name = read_string(file);
        if (rec_name == NULL) goto fail;

        int rc = table_insert(table, id, rec_name, price, stock);
        free(rec_name);   
        if (rc != 0) goto fail;
    }

    fclose(file);
    return table;

fail:
    fclose(file);
    table_destroy(table);
    return NULL;
}