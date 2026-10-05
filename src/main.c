#include <stdio.h>
#include "parser.h"

int main(void) {
    printf("VaultDB starting...\n\n");

    // 1. Test INSERT
    Command cmd_ins = parse_command("INSERT INTO products VALUES (1, \"Keyboard\", 1200, 10);");
    if (cmd_ins.type == CMD_INSERT) {
        printf("Parsed INSERT:\n");
        printf("  Table: %s\n", cmd_ins.table_name);
        printf("  ID: %d, Name: %s, Price: %d, Stock: %d\n\n", 
               cmd_ins.id, cmd_ins.record_name, cmd_ins.price, cmd_ins.stock);
    }

    // 2. Test SELECT
    Command cmd_sel = parse_command("SELECT * FROM products;");
    if (cmd_sel.type == CMD_SELECT) {
        printf("Parsed SELECT:\n");
        printf("  Table: %s\n\n", cmd_sel.table_name);
    }

    // 3. Test UPDATE
    Command cmd_upd = parse_command("UPDATE products SET price = 999 WHERE id = 1;");
    if (cmd_upd.type == CMD_UPDATE) {
        printf("Parsed UPDATE:\n");
        printf("  Table: %s\n", cmd_upd.table_name);
        printf("  Set %s = %d\n", cmd_upd.set_column, cmd_upd.set_value_int);
        printf("  Where %s = %d\n\n", cmd_upd.where_column, cmd_upd.where_value);
    }

    // 4. Test DELETE
    Command cmd_del = parse_command("DELETE FROM products WHERE id = 1;");
    if (cmd_del.type == CMD_DELETE) {
        printf("Parsed DELETE:\n");
        printf("  Table: %s\n", cmd_del.table_name);
        printf("  Where %s = %d\n\n", cmd_del.where_column, cmd_del.where_value);
    }

    return 0;
}
