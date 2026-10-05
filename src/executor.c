#include "executor.h"
#include <stdio.h>
#include <string.h>

void execute_command(Command cmd, Table **active_table) {
    switch (cmd.type) {
        case CMD_INSERT:
            if (*active_table == NULL) {
                printf("Error: No table selected\n");
                break;
            }
            if (table_insert(*active_table, cmd.id, cmd.record_name, cmd.price, cmd.stock) == 0) {
                printf("Inserted 1 row.\n");
            } else {
                printf("Insert failed.\n");
            }
            break;

        case CMD_SELECT:
            if (*active_table == NULL) {
                printf("Error: No table selected\n");
                break;
            }
         
            printf("ID  | Name       | Price | Stock\n");
            printf("--------------------------------\n");
            for (int i = 0; i < (*active_table)->record_count; i++) {
                Record *r = &(*active_table)->records[i];
                printf("%-3d | %-10s | %-5d | %-5d\n", r->id, r->name, r->price, r->stock);
            }
            break;

        case CMD_UPDATE: {
            if (*active_table == NULL) {
                printf("No table.\n");
                break;
            }

            Record *rec = table_select(*active_table, cmd.where_value);
            if (rec == NULL) {
                printf("Record not found.\n");
                break;
            }

            if (strcmp(cmd.set_column, "price") == 0) {
                table_update(*active_table, cmd.where_value, cmd.set_value_int, rec->stock);
                printf("Updated price.\n");
            } else if (strcmp(cmd.set_column, "stock") == 0) {
                table_update(*active_table, cmd.where_value, rec->price, cmd.set_value_int);
                printf("Updated stock.\n");
            } else {
                printf("Unknown column.\n");
            }
            break;
        }

        case CMD_DELETE:
            if (*active_table == NULL) {
                printf("No table.\n");
                break;
            }
            if (table_delete(*active_table, cmd.where_value) == 0) {
                printf("Deleted 1 row.\n");
            } else {
                printf("Record not found.\n");
            }
            break;

        case CMD_CREATE_TABLE:
            
            if (*active_table != NULL) {
                table_destroy(*active_table);
            }
            *active_table = table_create(cmd.table_name, 4);
            if (*active_table == NULL) {
                printf("Failed to create table.\n");
            } else {
                printf("Created table.\n");
            }
            break;

        default:
            printf("Unrecognized command.\n");
            break;
    }
}
