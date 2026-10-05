#ifndef STORAGE_H
#define STORAGE_H

#include "table.h"

int storage_save_table(Table *table, const char *filename);
Table *storage_load_table(const char *filename);

#endif 


