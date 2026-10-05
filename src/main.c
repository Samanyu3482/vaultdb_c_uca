#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "table.h"
#include "executor.h"
#include "storage.h"

int main(void) {
    printf("VaultDB version 1.0. Type .exit to quit.\n");

    Table *active_table = NULL;

    while (1) {
        printf("vaultdb> ");

        char input_buffer[512];
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf("Error reading input.\n");
            continue;
        }

        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        if (strlen(input_buffer) == 0) {
            continue;
        }

        /* Meta-commands */
        if (strcmp(input_buffer, ".exit") == 0) {
            break;
        }

        if (strcmp(input_buffer, ".save") == 0) {
            if (active_table != NULL) {
                storage_save_table(active_table, "data/vault.db");
                printf("Saved to data/vault.db\n");
            } else {
                printf("No active table to save.\n");
            }
            continue;
        }

        if (strcmp(input_buffer, ".load") == 0) {
            Table *loaded = storage_load_table("data/vault.db");
            if (loaded != NULL) {
                if (active_table != NULL) table_destroy(active_table);
                active_table = loaded;
                printf("Loaded from data/vault.db\n");
            } else {
                printf("Failed to load. Existing table unchanged.\n");
            }
            continue;
        }

        /* SQL queries */
        Command cmd = parse_command(input_buffer);
        if (cmd.type == CMD_UNKNOWN) {
            printf("Unrecognized command.\n");
        } else {
            execute_command(cmd, &active_table);
        }
    }

    if (active_table != NULL) {
        table_destroy(active_table);
    }
    return 0;
}
