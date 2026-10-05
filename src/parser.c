#include "parser.h"
#include <stdio.h>
#include <string.h>

Command parse_command(const char *query) {
    Command cmd = {0};
    cmd.type = CMD_UNKNOWN;

    if (query == NULL) return cmd;

    if (sscanf(query, "INSERT INTO %255s VALUES (%d, \"%255[^\"]\", %d, %d);",
               cmd.table_name, &cmd.id, cmd.record_name, &cmd.price, &cmd.stock) == 5) {
        cmd.type = CMD_INSERT;
        return cmd;
    }

    if (sscanf(query, "SELECT * FROM %255[^;];", cmd.table_name) == 1) {
        cmd.type = CMD_SELECT;
        return cmd;
    }

        
    if (sscanf(query, "UPDATE %255s SET %255s = %d WHERE %255s = %d;",
               cmd.table_name, cmd.set_column, &cmd.set_value_int, 
               cmd.where_column, &cmd.where_value) == 5) {
        cmd.type = CMD_UPDATE;
        cmd.has_where = 1;
        return cmd;
    }

  
    if (sscanf(query, "DELETE FROM %255s WHERE %255s = %d;",
               cmd.table_name, cmd.where_column, &cmd.where_value) == 3) {
        cmd.type = CMD_DELETE;
        cmd.has_where = 1;
        return cmd;
    }

    return cmd;
}
