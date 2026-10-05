# Document 5: Known Limitations and Future Work

## Table of Contents
1. [Parser Limitations](#1-parser-limitations)
2. [Query Feature Limitations](#2-query-feature-limitations)
3. [Storage Limitations](#3-storage-limitations)
4. [Architecture Limitations](#4-architecture-limitations)
5. [Robustness Limitations](#5-robustness-limitations)

---

## 1. Parser Limitations

### 1.1 Case-sensitive keywords

**Current behaviour:** `CREATE TABLE users;` works. `create table users;` prints "Unrecognized command."

**Why:** `sscanf` does exact character-by-character matching. Lowercase letters do not match uppercase format strings.

**Fix:** Before calling `parse_command`, convert the keyword portion of the input to uppercase. The simplest approach is to copy the input into a buffer and run `toupper` on each character (from `<ctype.h>`) before passing it to `sscanf`. A cleaner fix would be to implement a proper tokenizer that normalizes keywords.

---

### 1.2 No error messages for malformed queries

**Current behaviour:** `INSERT INTO products VALUES (1, Keyboard, 1200, 10);` (missing quotes around name) silently returns CMD_UNKNOWN and prints "Unrecognized command."

**Why:** `sscanf` either fully matches the pattern or returns a count less than expected. There is no partial-match feedback.

**Fix:** Add a pre-check using `strncmp` to detect which command keyword was used, then report a more specific error: "INSERT format error: expected INSERT INTO table VALUES (id, "name", price, stock);"

---

### 1.3 sscanf allows trailing characters

**Current behaviour:** `INSERT INTO products VALUES (1, "Mouse", 500, 10); some garbage here` still parses successfully. `sscanf` stops after matching all 5 fields and ignores the rest.

**Why:** `sscanf` is not a full parser; it does not verify that the entire input was consumed.

**Fix:** After a successful `sscanf` match, use the `%n` specifier to get the number of characters consumed, then check that the remaining input (excluding whitespace) is empty.

---

### 1.4 CREATE TABLE ignores the table name when executing

**Current behaviour:** `CREATE TABLE products;` creates a table correctly. But `INSERT INTO users VALUES ...` after that will still insert into the table named "products" (because the executor only checks if a table exists, not if the table name matches).

**Why:** The executor does not validate `cmd.table_name` against `(*active_table)->name` for INSERT, SELECT, UPDATE, or DELETE.

**Fix:** Add a name check in the executor for all commands:
```c
if (strcmp(cmd.table_name, (*active_table)->name) != 0) {
    printf("Error: No table named '%s' is active.\n", cmd.table_name);
    break;
}
```

---

## 2. Query Feature Limitations

### 2.1 SELECT does not support WHERE

**Current behaviour:** `SELECT * FROM products;` always prints every record.

**Why:** The `has_where`, `where_column`, and `where_value` fields in the Command struct are populated but the executor's CMD_SELECT case never reads them.

**Fix:** In the CMD_SELECT executor case, check `cmd.has_where`. If true, only print records where `records[i].id == cmd.where_value`.

---

### 2.2 UPDATE only supports `price` and `stock` columns

**Current behaviour:** `UPDATE products SET name = "Laptop Pro" WHERE id = 1;` prints "Unknown column."

**Why:** The executor uses `strcmp(cmd.set_column, "price")` and `strcmp(cmd.set_column, "stock")`. Name and id are not handled.

**Fix:** Add cases for "name" (which would require freeing the old name, mallocing a new one, and copying the new value). Updating "id" would also need a check to prevent duplicate IDs.

---

### 2.3 DELETE always deletes by id

**Current behaviour:** You can only delete `WHERE id = n`. You cannot delete `WHERE name = "..."` or `WHERE price = n`.

**Why:** The executor calls `table_delete(*active_table, cmd.where_value)` and `table_delete` only searches by id.

**Fix:** Add a `table_delete_by_name` function and an additional `strcmp(cmd.where_column, "id")` check in the executor to dispatch to the correct delete function.

---

### 2.4 No support for multiple tables

**Current behaviour:** Only one table can be in memory at a time. `CREATE TABLE` destroys the current table.

**Why:** `active_table` in `main.c` is a single pointer, not an array or linked list of tables.

**Fix:** Replace `Table *active_table` with a `Table *tables[MAX_TABLES]` array and add a `USE tablename;` command to switch the active table. The storage format would also need to change to support multiple tables per file.

---

### 2.5 No support for duplicate ID detection

**Current behaviour:** You can insert two records with the same id. `table_select` will always find only the first one; the second is invisible to SELECT, UPDATE, and DELETE.

**Why:** `table_insert` does no uniqueness check before inserting.

**Fix:** Before inserting, call `table_select(table, id)`. If it returns non-NULL, reject the insert with an error.

---

## 3. Storage Limitations

### 3.1 .load destroys the current table before confirming success

**Current behaviour:** If `data/vault.db` is corrupted, `.load` destroys the in-memory table first, then fails to load. The user loses their data with no recovery.

**Why:** `table_destroy` is called unconditionally before `storage_load_table`.

**Fix:**
```c
Table *loaded = storage_load_table("data/vault.db");
if (loaded != NULL) {
    if (active_table != NULL) table_destroy(active_table);
    active_table = loaded;
    printf("Loaded from data/vault.db\n");
} else {
    printf("Failed to load. Existing table unchanged.\n");
}
```

---

### 3.2 Hardcoded filename

**Current behaviour:** `.save` always saves to `data/vault.db`. `.load` always loads from the same path. The user cannot specify a filename.

**Fix:** Parse the filename from the meta-command: `.save myfile.db` and `.load myfile.db`.

---

### 3.3 fwrite return values are not checked

**Current behaviour:** If the disk is full during `.save`, the binary file may be written partially. The function returns 0 (success) even though the write failed.

**Fix:** Check every `fwrite` call and return -1 if fewer items than requested were written. Also consider writing to a temporary file first and renaming it on success (atomic save), to prevent a partial write from corrupting the saved file.

---

### 3.4 Binary format is not portable across platforms or compiler versions

**Current behaviour:** The binary file uses raw `int` bytes. An `int` is 4 bytes on most modern systems, but this is not guaranteed by the C standard. On a system with different int sizes, the file would be unreadable.

**Fix:** Use fixed-size types from `<stdint.h>` like `int32_t` for all fields written to disk. This guarantees the format is the same on every platform.

---

## 4. Architecture Limitations

### 4.1 Only one command can be typed at a time

**Current behaviour:** You must type one query and press Enter. You cannot batch commands.

**Fix:** Accept a script file as a command-line argument and process each line as a command. Standard databases support `vaultdb < script.sql`.

---

### 4.2 No unit tests

**Current behaviour:** The `tests/` folder is empty. Correctness has only been verified by manual REPL interaction.

**Fix:** Write test cases in `tests/test_table.c` that directly call `table_create`, `table_insert`, `table_select`, etc. with known inputs and assert expected outputs. Add a `make test` target to the Makefile.

---

## 5. Robustness Limitations

### 5.1 Buffer overflow risk in sscanf

**Current behaviour:** `%255s` and `%255[^\"]` limit sscanf to 255 characters, which fits in the 256-byte char arrays.

**Status:** This is correctly handled. The `255` limit is one less than 256 to leave room for the null terminator. No overflow is possible.

---

### 5.2 Ctrl+C does not trigger graceful cleanup

**Current behaviour:** Pressing Ctrl+C sends SIGINT and kills the process immediately. The `table_destroy` in `main.c` does not run. Unsaved data is lost.

**Fix:** Install a signal handler using `signal(SIGINT, handler)` from `<signal.h>`. The handler would set a global `volatile int running = 0` flag, and the `while(1)` loop would check `while(running)` to exit cleanly.

---

### 5.3 No input length validation

**Current behaviour:** The input buffer is 512 bytes. If the user types more than 511 characters, `fgets` will truncate silently. The remaining characters will be read in the next loop iteration as a second command.

**Fix:** After `fgets`, check if the last character before `\0` is `\n`. If not, the line was truncated. Flush stdin with `while (getchar() != '\n');` and print an error.
