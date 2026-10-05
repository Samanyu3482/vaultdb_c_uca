# Document 2: File-by-File Reference

## Table of Contents
1. [record.h](#1-recordh)
2. [record.c](#2-recordc)
3. [table.h](#3-tableh)
4. [table.c](#4-tablec)
5. [storage.h](#5-storageh)
6. [storage.c](#6-storagec)
7. [parser.h](#7-parserh)
8. [parser.c](#8-parserc)
9. [executor.h](#9-executorh)
10. [executor.c](#10-executorc)
11. [main.c](#11-mainc)
12. [Makefile](#12-makefile)

---

## 1. record.h

**File:** `include/record.h`

### Purpose
Declares the most fundamental data type in VaultDB: the Record struct. A Record is a single row of data. This header also declares the two functions that manage a Record's lifecycle. Every other layer of the system depends on this definition.

### Dependencies
None. This file includes no other headers. It intentionally has no dependencies so it can be included by anything without causing circular includes.

### Data Structures

**`Record` struct (typedef'd)**

| Field | Type | Description |
|---|---|---|
| id | int | Unique integer identifier for this row |
| name | char * | Pointer to a heap-allocated string for the item name |
| price | int | Integer price of the item |
| stock | int | Integer stock count |

The `name` field is a pointer, not a fixed-size array. This means the memory for the name string is not stored inside the struct itself — it lives somewhere else on the heap. The struct only holds the address of that memory.

### Functions

| Function | Signature | Returns |
|---|---|---|
| record_create | `Record *record_create(int id, const char *name, int price, int stock)` | Pointer to new Record, or NULL on failure |
| record_destroy | `void record_destroy(Record *record)` | Nothing |

### Header Guard
```c
#ifndef RECORD_H
#define RECORD_H
...
#endif
```
The `#ifndef` / `#define` / `#endif` pattern is called a header guard. If this header is included by multiple files, the preprocessor will only process it once, preventing duplicate struct definitions that would cause compiler errors.

### Viva Questions

**Q: Why is `name` a `char *` and not `char name[256]`?**
A: A `char *` only occupies 8 bytes (on 64-bit systems) regardless of string length. A `char name[256]` always uses 256 bytes even if the name is 3 characters long. Since each Table holds many Records, this memory saving matters. The downside is that the code must manually `malloc` and `free` the name string.

**Q: What is a header guard and why is it needed?**
A: A header guard prevents a header file from being processed more than once during compilation. If `file_a.c` and `file_b.c` both include `record.h`, and `file_a.c` also includes `file_b.c`, the compiler would see `record.h` twice. Without the guard, this causes a "type redefinition" error. The guard's `#ifndef` check skips the file body on the second encounter.

**Q: Why does `record_create` take `const char *name` instead of `char *name`?**
A: The `const` keyword tells the caller: "I promise not to modify the string you pass me." This allows the function to accept string literals (like `"Keyboard"`) which cannot be modified, and it makes the interface safer and clearer.

---

## 2. record.c

**File:** `src/record.c`

### Purpose
Implements the two functions declared in `record.h`. This is where heap memory is actually allocated and freed for individual Record objects.

### Dependencies

| Header | Why it is included |
|---|---|
| `"record.h"` | To get the Record struct definition and function declarations |
| `<stdlib.h>` | For `malloc` and `free` |
| `<string.h>` | For `strlen` and `strcpy` |

### Functions

---

#### `record_create`

**Signature:** `Record *record_create(int id, const char *name, int price, int stock)`

**Returns:** A pointer to the new Record on success. NULL if any memory allocation failed.

**Step-by-step:**
1. `malloc(sizeof(Record))` — allocates one Record-sized block on the heap. `sizeof(Record)` calculates the exact byte size of the struct automatically.
2. NULL check — if malloc returned NULL (out of memory), the function returns NULL immediately. Nothing has been allocated that needs to be freed yet.
3. Assigns `id`, `price`, `stock` directly to the struct fields.
4. `malloc(strlen(name) + 1)` — allocates a separate block of memory for the name string. `strlen(name)` counts the characters, and `+ 1` makes room for the null terminator `\0`.
5. NULL check — if this second malloc fails, the Record struct allocated in step 1 is freed before returning NULL. This is partial-failure cleanup and prevents a memory leak.
6. `strcpy(record->name, name)` — copies the string from the caller's buffer into the newly allocated block.
7. Returns the pointer.

**Memory operations:**

| Operation | What | Why | Freed where |
|---|---|---|---|
| `malloc(sizeof(Record))` | The Record struct itself | Needs heap lifetime; must outlive the function call | `record_destroy` → `free(record)` |
| `malloc(strlen(name)+1)` | The name string | Strings of variable length must be heap-allocated | `record_destroy` → `free(record->name)` |

---

#### `record_destroy`

**Signature:** `void record_destroy(Record *record)`

**Returns:** Nothing.

**Step-by-step:**
1. NULL check — if the passed pointer is NULL, return immediately. This prevents a crash if the caller calls destroy on a failed create.
2. `free(record->name)` — frees the separately allocated name string FIRST.
3. `free(record)` — frees the Record struct itself.

**Why this order?** After `free(record)`, the pointer `record` is invalid and `record->name` is undefined behaviour to read. So the name must be freed while `record` is still a valid pointer.

### C Concepts Used

**`->` operator:** Used to access a field through a pointer. `record->id` is equivalent to `(*record).id`. Because `record` is a pointer to a struct, you use `->` instead of `.`.

**Two-step malloc pattern:** The Record and its name string are two separate heap allocations. This is necessary because the size of the name is not known at compile time — it depends on what the user types.

### Viva Questions

**Q: Why must `name` be freed before `record` in `record_destroy`?**
A: After `free(record)`, the memory of the struct is returned to the system. Reading `record->name` after that is undefined behaviour — the pointer might still look valid but the memory could have been reused. Freeing the name first, while the struct is still valid, is the correct order.

**Q: Why is `record_destroy` never actually called in VaultDB's current implementation?**
A: VaultDB stores Records by value inside the Table's array, not as individual heap-allocated pointers. So the Records are freed collectively when the array block is freed, and only their `name` strings need individual frees. `record_create` is also unused in the final program for the same reason. Both exist as a clean, standalone API and are used directly in the Table layer.

**Q: What happens if `malloc(strlen(name) + 1)` fails but `malloc(sizeof(Record))` already succeeded?**
A: Without the `free(record)` cleanup line, the Record block would be leaked — it was allocated, but the function returns NULL and nobody has a pointer to it anymore so it can never be freed. The code correctly frees `record` before returning NULL to prevent this leak.

**Q: What does `sizeof(Record)` actually evaluate to?**
A: It is computed at compile time. Given the struct has two `int` fields (4 bytes each) and one `char *` (8 bytes on 64-bit), plus possible padding, it is typically 24 bytes on a 64-bit Mac. The programmer never needs to calculate this manually.

---

## 3. table.h

**File:** `include/table.h`

### Purpose
Declares the Table struct — the central data structure of VaultDB — and all the functions that operate on it. The Table is an in-memory dynamic array of Record structs.

### Dependencies

| Header | Why |
|---|---|
| `"record.h"` | Table contains a `Record *` field, so the compiler must know what Record is |

### Data Structures

**`Table` struct (typedef'd)**

| Field | Type | Description |
|---|---|---|
| name | char * | Heap-allocated string holding the table's name (e.g. "products") |
| records | Record * | Pointer to the start of a heap-allocated array of Record structs |
| record_count | int | Number of records currently stored in the array |
| capacity | int | Total number of Record slots currently allocated |

**The dynamic array concept:** `records` is a pointer to a contiguous block of memory. If `capacity` is 4, that block holds 4 Record structs back to back. `record_count` tracks how many of those 4 slots contain actual data. When `record_count == capacity`, the array is full and must be grown before the next insert.

### Functions Summary

| Function | Purpose |
|---|---|
| `table_create` | Allocates and initializes a new Table |
| `table_destroy` | Frees all memory owned by the Table |
| `table_insert` | Adds a record; grows the array if needed |
| `table_select` | Finds a record by id; returns a pointer to it |
| `table_update` | Changes price or stock of a record found by id |
| `table_delete` | Removes a record by id and shifts the array |

### Viva Questions

**Q: What is the difference between `record_count` and `capacity`?**
A: `capacity` is how many Record-sized slots are allocated in the `records` block. `record_count` is how many of those slots contain actual data. The analogy: a parking lot with 10 spaces (capacity) that currently has 6 cars (record_count).

**Q: Why store `Record` structs by value in the array instead of `Record *` pointers?**
A: Storing by value (`Record *records`) means all Records are in one contiguous block of memory. This is cache-friendly — the CPU can scan through them quickly. Storing pointers (`Record **records`) would require a separate heap allocation per record, which is slower and more complex to manage.

---

## 4. table.c

**File:** `src/table.c`

### Purpose
Implements all six Table functions. This is the core data management layer of VaultDB.

### Dependencies

| Header | Why |
|---|---|
| `"table.h"` | Gets both Table and Record definitions |
| `<stdlib.h>` | For malloc, realloc, free |
| `<string.h>` | For strlen, strcpy |

### Functions

---

#### `table_create`

**Signature:** `Table *table_create(const char *name, int initial_capacity)`

**Returns:** Pointer to new Table, or NULL on any allocation failure.

**Three-malloc chain:**
1. `malloc(sizeof(Table))` — allocates the Table struct. NULL check: return NULL.
2. `malloc(strlen(name) + 1)` — allocates the name string. NULL check: free(table), return NULL.
3. `malloc(initial_capacity * sizeof(Record))` — allocates the records array. NULL check: free(table->name), free(table), return NULL.

The key discipline here: each failure case frees everything allocated in the steps before it. If the third malloc fails, both the table struct and the name string must be freed.

Sets `record_count = 0` and `capacity = initial_capacity`, then returns the pointer.

**Memory owned by Table:**

| Allocation | Freed in |
|---|---|
| The Table struct | `table_destroy` → `free(table)` |
| `table->name` string | `table_destroy` → `free(table->name)` |
| `table->records` array | `table_destroy` → `free(table->records)` |
| Each `records[i].name` string | `table_destroy` loop → `free(records[i].name)` |

---

#### `table_destroy`

**Signature:** `void table_destroy(Table *table)`

**Returns:** Nothing.

**Step-by-step:**
1. NULL check — return immediately if table is NULL.
2. Loop through every record from 0 to `record_count - 1` and free each `records[i].name`. This uses `.name` (dot notation) not `->name` because `records[i]` is a Record struct value, not a pointer.
3. `free(table->records)` — frees the entire array block.
4. `free(table->name)` — frees the table's name string.
5. `free(table)` — frees the Table struct itself.

Why this order: innermost allocations are freed first, working outward.

---

#### `table_insert`

**Signature:** `int table_insert(Table *table, int id, const char *name, int price, int stock)`

**Returns:** 0 on success, -1 on failure.

**Step-by-step:**
1. NULL check for table and name.
2. **Growth check:** If `record_count == capacity`:
   - Calculate `newCapacity = (capacity == 0) ? 4 : capacity * 2`. The edge case of `capacity == 0` is handled gracefully by jumping to 4 instead of doubling 0.
   - `Record *temp = realloc(table->records, newCapacity * sizeof(Record))` — asks the OS to resize the block. Result stored in `temp`, never directly in `table->records`.
   - If `temp == NULL`, return -1. The old `table->records` is still valid because we used a temp variable.
   - If not NULL, `table->records = temp` and `table->capacity = newCapacity`.
3. `Record *slot = &table->records[table->record_count]` — gets a pointer to the next empty slot.
4. Allocates and copies the name string into `slot->name`.
5. Sets `slot->id`, `slot->price`, `slot->stock`.
6. `table->record_count++` — only incremented after all allocations succeed.
7. Returns 0.

**The realloc safety pattern:** Writing `ptr = realloc(ptr, size)` is dangerous. If realloc returns NULL, you have overwritten your only reference to the old memory, leaking it. Always use a temporary pointer.

---

#### `table_select`

**Signature:** `Record *table_select(Table *table, int id)`

**Returns:** Pointer to the matching Record (inside the array), or NULL if not found.

**Step-by-step:**
1. NULL check for table.
2. Linear search: loop from 0 to `record_count - 1`. If `records[i].id == id`, return `&records[i]`.
3. If no match found, return NULL.

**Important:** Returns a pointer into the live array. If the caller then calls `table_insert` and realloc moves the array, this pointer becomes invalid (dangling). In the current code, the executor never mixes a live `table_select` result with a subsequent insert, so this is safe in practice.

---

#### `table_update`

**Signature:** `int table_update(Table *table, int id, int new_price, int new_stock)`

**Returns:** 0 on success, -1 if the id was not found.

Calls `table_select` to find the record. If found, sets `rec->price = new_price` and `rec->stock = new_stock` directly through the pointer. Returns -1 if select returned NULL.

Note: `table_update` requires both `new_price` and `new_stock` to be provided even if only one is changing. The executor handles this by reading the current value of the unchanged field first and passing it back in.

---

#### `table_delete`

**Signature:** `int table_delete(Table *table, int id)`

**Returns:** 0 on success, -1 if the id was not found.

**Step-by-step:**
1. NULL check for table.
2. Loop to find the matching id.
3. When found at index `i`:
   - `free(table->records[i].name)` — frees the name string BEFORE the slot is overwritten.
   - Inner loop: copies `records[j] = records[j+1]` for j from i to `record_count - 2`. This shifts every element after the deleted one left by one position.
   - `record_count--`.
   - Returns 0.
4. If the loop ends without finding the id, returns -1.

**Why free name before shifting?** The shift operation copies the struct value at `j+1` over the slot at `j`. The struct contains the `name` pointer. If we freed `records[i].name` after shifting, we would be freeing the name of `records[i+1]` (since the shift already moved it there), causing a double-free and corruption.

### Viva Questions

**Q: Why does `table_insert` double the capacity instead of adding 1 each time?**
A: Doubling is an amortized O(1) strategy. If you add 1 each time, every single insert requires a realloc and a full copy of the existing data — that's O(n) per insert. With doubling, reallocs happen logarithmically rarely. The total cost of n inserts is O(n), not O(n²).

**Q: What happens if `realloc` fails inside `table_insert`?**
A: The function returns -1. The table is unchanged — `table->records` still points to the old, valid array. No data is lost.

**Q: Why does `table_insert` increment `record_count` at the very end?**
A: If `malloc` for the name string fails after we already incremented the count, the table would believe there is a record at index `record_count-1` but that slot's name pointer would be uninitialized or NULL. By incrementing only after all allocations succeed, we ensure the count always reflects only successfully completed records.

**Q: In `table_delete`, why does the inner shifting loop stop at `record_count - 2`?**
A: Inside the loop, we access index `j+1`. If j reached `record_count - 1`, then `j+1` would be `record_count`, which is past the end of valid data. Stopping at `record_count - 2` makes the last read `records[record_count - 1]`, which is the last valid element.

**Q: `table_select` returns a pointer into the live records array. When is this dangerous?**
A: If after calling `table_select`, you call `table_insert` and that insert triggers a `realloc`, the array could move to a completely different address in memory. The pointer returned by `table_select` would then point at freed memory — a dangling pointer. In the current executor, `table_select` is only called in `CMD_UPDATE` and immediately used before any insert could happen, so it is safe in practice but would be a real bug in a more complex system.

---

## 5. storage.h

**File:** `include/storage.h`

### Purpose
Declares the two functions of the Storage Engine: one to serialize a Table to a binary file on disk, and one to deserialize it back. This is what gives VaultDB persistence — data survives program restarts.

### Dependencies

| Header | Why |
|---|---|
| `"table.h"` | The function signatures reference Table, so the compiler needs to know its definition |

### Functions

| Function | Signature | Returns |
|---|---|---|
| `storage_save_table` | `int storage_save_table(Table *table, const char *filename)` | 0 on success, -1 on failure |
| `storage_load_table` | `Table *storage_load_table(const char *filename)` | Pointer to loaded Table, or NULL on failure |

---

## 6. storage.c

**File:** `src/storage.c`

### Purpose
Implements binary file I/O for the Table. "Binary format" means the raw bytes of the integers and strings are written directly to the file, not as human-readable text.

### Dependencies

| Header | Why |
|---|---|
| `"storage.h"` | Gets Table and Record definitions, plus function declarations |
| `<stdio.h>` | For FILE, fopen, fwrite, fread, fclose |
| `<stdlib.h>` | For malloc, free |
| `<string.h>` | For strlen |

### Functions

---

#### `storage_save_table`

**Signature:** `int storage_save_table(Table *table, const char *filename)`

**Returns:** 0 on success, -1 on any failure.

**File format written (in order):**
```
[4 bytes: table name length (including null terminator)]
[N bytes: table name string]
[4 bytes: record_count]
For each record:
    [4 bytes: id]
    [4 bytes: price]
    [4 bytes: stock]
    [4 bytes: name length (including null terminator)]
    [M bytes: name string]
```

**Step-by-step:**
1. NULL checks for table and filename.
2. `fopen(filename, "wb")` — opens the file for writing in binary mode. Creates the file if it does not exist. Overwrites if it does.
3. Writes the table name length (as an int) then the name string using `fwrite`.
4. Writes `record_count`.
5. Loops through every record and writes id, price, stock, name length, and name string.
6. `fclose(file)`.
7. Returns 0.

**Why write string lengths before strings?** When reading the file back, we need to know how many bytes to `malloc` before we can read the string. The length tells us. Without it, we would not know where one string ends and the next field begins.

**`fwrite` usage:**
- `fwrite(&name_len, sizeof(int), 1, file)` — writes 1 item of size `sizeof(int)` from the address of `name_len`. The `&` is required because `fwrite` takes a pointer.
- `fwrite(table->name, sizeof(char), name_len, file)` — writes `name_len` items of size 1 byte from the string. No `&` needed because `table->name` is already a pointer.

---

#### `read_string` (static helper)

**Signature:** `static char *read_string(FILE *file)`

**Returns:** Heap-allocated string read from the file, or NULL on error.

The `static` keyword means this function is private to `storage.c`. No other file can call it. This is good practice — it hides implementation details.

**Step-by-step:**
1. Reads an `int` (the length) from the file.
2. If the read fails or the length is not positive, returns NULL.
3. `malloc(len)` — allocates a buffer of exactly that size.
4. Reads `len` bytes from the file into the buffer. Checks that exactly `len` bytes were read.
5. If the read fails, frees the buffer and returns NULL.
6. Returns the buffer pointer.

The caller is responsible for `free`-ing the returned string.

---

#### `storage_load_table`

**Signature:** `Table *storage_load_table(const char *filename)`

**Returns:** A fully reconstructed Table pointer, or NULL on any failure.

**Step-by-step:**
1. NULL check for filename.
2. `fopen(filename, "rb")` — opens for reading in binary mode. Returns NULL if the file does not exist.
3. Reads the table name using `read_string`.
4. Reads `record_count` as an int.
5. Validates both (name must not be NULL, record_count must be non-negative).
6. `table_create(table_name, record_count)` — creates the Table. `free(table_name)` immediately after, because `table_create` makes its own copy.
7. Loop `record_count` times: reads id, price, stock, then reads the name via `read_string`. Calls `table_insert` to add the record. `free(rec_name)` immediately after because `table_insert` makes its own copy.
8. On any failure inside the loop, jumps to `fail:` label.
9. `fail:` label: closes the file, calls `table_destroy` to free partially built table, returns NULL.
10. On success: closes file, returns table.

**The `goto fail` pattern:**
```c
fail:
    fclose(file);
    table_destroy(table);
    return NULL;
```
This is standard C error-handling. Instead of duplicating the cleanup code at every possible failure point, every failure jumps to the single cleanup label. This is the exact pattern used in the Linux kernel for resource cleanup.

### Viva Questions

**Q: Why is the file opened with "wb" and "rb" instead of "w" and "r"?**
A: The `b` flag opens the file in binary mode. Without it, on Windows, the C library would translate `\n` characters to `\r\n` during writes, corrupting integer data that happens to contain those byte values. Even on Mac/Linux (where it makes no difference), using binary mode is correct and explicit for raw byte I/O.

**Q: Why is `read_string` declared `static`?**
A: `static` at file scope limits the function's visibility to the current translation unit (the .c file). It cannot be called from other .c files. Since `read_string` is an implementation detail of `storage.c` that nothing else needs, making it static prevents accidental use elsewhere and avoids polluting the global namespace.

**Q: Why is `record_count` used as the initial capacity for `table_create` during a load?**
A: Because we know exactly how many records the file contains, there is no need to start with extra capacity and grow. Passing `record_count` as the initial capacity allocates exactly the right amount of memory in one shot, with no realloc needed.

**Q: What is the `goto fail` pattern and why is it used here?**
A: The load function has many sequential steps that each can fail. If failure at any step means you need to do the same three cleanup actions (fclose, table_destroy, return NULL), you could either copy those three lines at every failure point (code duplication) or jump to a single shared cleanup block using goto. The goto approach is cleaner and less error-prone. It is the standard pattern in the Linux kernel for this exact situation.

**Q: After `table_create` succeeds, why is `table_name` immediately freed?**
A: `table_create` internally calls `malloc` and `strcpy` to make its own copy of the name string. So after `table_create` returns, the `table_name` buffer allocated by `read_string` is no longer needed. Keeping it around would be a memory leak.

---

## 7. parser.h

**File:** `include/parser.h`

### Purpose
Defines the `CommandType` enum and the `Command` struct, which together represent a parsed query as a structured C object. Also declares `parse_command`, the single entry point to the parsing system.

### Data Structures

**`CommandType` enum**

| Value | Represents |
|---|---|
| CMD_UNKNOWN | No pattern was matched; the query is unrecognized |
| CMD_CREATE_TABLE | `CREATE TABLE name;` |
| CMD_INSERT | `INSERT INTO name VALUES (...);` |
| CMD_SELECT | `SELECT * FROM name;` |
| CMD_UPDATE | `UPDATE name SET col = val WHERE id = n;` |
| CMD_DELETE | `DELETE FROM name WHERE id = n;` |

An enum assigns an integer to each name automatically (CMD_UNKNOWN = 0, CMD_CREATE_TABLE = 1, etc.). The programmer writes readable names; the compiler stores integers.

**`Command` struct**

| Field | Type | Used by |
|---|---|---|
| type | CommandType | All commands |
| table_name[256] | char array | All commands |
| id | int | INSERT |
| record_name[256] | char array | INSERT |
| price | int | INSERT |
| stock | int | INSERT |
| has_where | int | UPDATE, DELETE (1 = WHERE present) |
| where_column[256] | char array | UPDATE, DELETE |
| where_value | int | UPDATE, DELETE |
| set_column[256] | char array | UPDATE |
| set_value_int | int | UPDATE |

**Why fixed-size char arrays instead of `char *`?** The Command struct is temporary — it only lives for the duration of one query cycle. Using fixed arrays like `char[256]` avoids any malloc/free for the Command itself. The struct is returned by value from `parse_command`, which is valid and efficient because fixed arrays are copied by value.

### Viva Questions

**Q: Why does `parse_command` return `Command` by value instead of returning `Command *`?**
A: Returning a pointer to a local struct would be undefined behaviour — the local struct is destroyed when the function returns, leaving a dangling pointer. Returning by value copies the struct to the caller, which is safe. Since Command only contains ints and fixed char arrays (no heap pointers), copying is cheap and correct.

**Q: Why is `CMD_UNKNOWN` the first value in the enum?**
A: Enum values start at 0 by default. When `Command cmd = {0}` is used to zero-initialize the struct, `cmd.type` is set to 0, which is `CMD_UNKNOWN`. This means a zero-initialized Command automatically represents an unknown/unset command, which is the safe default.

---

## 8. parser.c

**File:** `src/parser.c`

### Purpose
Implements `parse_command`. Converts a raw user-typed string into a `Command` struct by pattern-matching with `sscanf`.

### Dependencies

| Header | Why |
|---|---|
| `"parser.h"` | Gets CommandType, Command definitions |
| `<stdio.h>` | For sscanf |
| `<string.h>` | For string utilities (though not explicitly used in current code) |

### Functions

#### `parse_command`

**Signature:** `Command parse_command(const char *query)`

**Returns:** A `Command` struct with `type` set to the matched command, or `CMD_UNKNOWN` if nothing matched.

**Step-by-step:**
1. `Command cmd = {0}` — zero-initializes all fields. `cmd.type` starts as `CMD_UNKNOWN` (which equals 0).
2. `cmd.type = CMD_UNKNOWN` — explicit assignment for clarity.
3. NULL check on query.
4. Tries to match INSERT with `sscanf(...) == 5`. If it matches all 5 fields, returns immediately.
5. Tries to match SELECT with `sscanf(...) == 1`.
6. Tries to match UPDATE with `sscanf(...) == 5`.
7. Tries to match DELETE with `sscanf(...) == 3`.
8. Tries to match CREATE TABLE with `sscanf(...) == 1`.
9. Returns cmd (still CMD_UNKNOWN if nothing matched).

**How `sscanf` works:**
```c
sscanf("INSERT INTO products VALUES (1, \"Keyboard\", 1200, 10);",
       "INSERT INTO %255s VALUES (%d, \"%255[^\"]\", %d, %d);",
       cmd.table_name, &cmd.id, cmd.record_name, &cmd.price, &cmd.stock)
```
- `%255s` matches a word up to 255 characters (stops at whitespace).
- `%d` matches an integer.
- `%255[^\"]` matches up to 255 characters that are not a double-quote, which reads the quoted name string.
- `sscanf` returns the number of fields successfully assigned.
- Notice: `cmd.table_name` (an array) needs no `&`. `&cmd.id` (an int) needs `&`.

**Note on `CREATE TABLE` order:** The CREATE TABLE check appears at the bottom of the function, after UPDATE and DELETE. This is fine because none of the patterns conflict with each other. However, it would be cleaner to have it near the top.

### Viva Questions

**Q: Why does `sscanf` need `&` for integers but not for char arrays?**
A: `sscanf` needs a pointer to write into. For an integer variable like `cmd.id`, you pass `&cmd.id` to get its address. For a char array like `cmd.table_name`, the array name decays to a pointer to its first element automatically — passing `cmd.table_name` is already passing a pointer.

**Q: What does `%255[^\"]` mean in an sscanf format string?**
A: It is a character class. `[^\"]` means "any character that is not a double-quote." Reading stops when a double-quote is encountered. The `255` limits the read to 255 characters to prevent overflow into the 256-byte buffer. The surrounding `\"` in the format string matches the literal quote characters in the user's input.

**Q: What happens if the user types a query that partially matches a pattern?**
A: sscanf returns the count of fields that were successfully matched. If it's less than the expected count (e.g., 3 instead of 5 for INSERT), the `if` condition fails and the function falls through to the next pattern. The partially-filled fields are ignored.

**Q: Why is the parser case-sensitive?**
A: `sscanf` matches strings character by character. The format string `"INSERT INTO"` only matches the exact bytes `I`, `N`, `S`, `E`, `R`, `T`, ` `, `I`, `N`, `T`, `O`. A real database would convert the query to uppercase before parsing to support case-insensitive keywords.

---

## 9. executor.h

**File:** `include/executor.h`

### Purpose
Declares the single function `execute_command`, which bridges the parser output (a Command struct) to the table backend (the functions in table.c).

### Dependencies

| Header | Why |
|---|---|
| `"parser.h"` | To get the Command type |
| `"table.h"` | To get the Table type |

### The `Table **` Parameter

`execute_command` takes `Table **active_table`, a pointer to a pointer. This is required because:
- `execute_command` may need to assign a brand new Table to the caller's `active_table` variable (e.g., on CREATE TABLE or .load).
- In C, to modify a variable in the caller's scope, you must pass its address. Since `active_table` in `main` is a `Table *`, its address is `Table **`.
- Inside `execute_command`, `*active_table` dereferences to get the actual `Table *`.

---

## 10. executor.c

**File:** `src/executor.c`

### Purpose
Implements `execute_command`. Uses a `switch` statement on `cmd.type` to dispatch each command to the appropriate `table.c` function and print the result.

### Dependencies

| Header | Why |
|---|---|
| `"executor.h"` | Gets Command, Table, and function declaration |
| `<stdio.h>` | For printf |
| `<string.h>` | For strcmp (used in CMD_UPDATE) |

### Function: `execute_command`

**Signature:** `void execute_command(Command cmd, Table **active_table)`

**Returns:** Nothing. Results are printed directly to stdout.

**Per-case breakdown:**

**CMD_INSERT:**
- Checks `*active_table != NULL`.
- Calls `table_insert(*active_table, cmd.id, cmd.record_name, cmd.price, cmd.stock)`.
- Prints "Inserted 1 row." or "Insert failed."

**CMD_SELECT:**
- Checks `*active_table != NULL`.
- Loops through all records and prints a formatted table using `%-3d`, `%-10s`, `%-5d` format specifiers. The `-` means left-aligned. The number is the minimum field width.

**CMD_UPDATE:**
- The case block uses `{}` braces because it declares a local variable (`Record *rec`). In C, you cannot declare variables directly inside a `case` without braces; this would cause a "jump over initialization" error.
- Calls `table_select` to get the current record.
- Uses `strcmp(cmd.set_column, "price")` to decide whether to update price or stock.
- Passes the existing value for the unchanged field to `table_update`.

**CMD_DELETE:**
- Calls `table_delete(*active_table, cmd.where_value)`.
- Prints "Deleted 1 row." or "Record not found."

**CMD_CREATE_TABLE:**
- If a table already exists, calls `table_destroy(*active_table)` first to prevent a memory leak.
- `*active_table = table_create(cmd.table_name, 4)` — creates the new table with initial capacity 4.

**default:**
- Prints "Unrecognized command."

### Viva Questions

**Q: Why does `execute_command` take `Table **` instead of `Table *`?**
A: Because CMD_CREATE_TABLE must assign a brand new Table to the caller's pointer variable. In C, to modify a variable in the calling function, you must pass a pointer to it. Since the caller's variable is `Table *active_table`, a pointer to it is `Table **`. Inside the function, `*active_table = table_create(...)` writes through the double pointer to modify the caller's variable.

**Q: Why does the CMD_UPDATE case need curly braces around it?**
A: In C, it is not legal to declare a new variable in the middle of a `switch` block without wrapping the case in braces. Without braces, the compiler would allow a `goto` (like another case label) to jump over the variable declaration, which is undefined. The braces create a new scope that contains the declaration safely.

**Q: Why does CMD_UPDATE call `table_select` first before calling `table_update`?**
A: `table_update` requires both the new price AND the new stock. The user's query only provides one of them (e.g., `SET price = 999`). The executor uses `table_select` to find the record and read the current value of the field that is NOT being changed, so it can pass it unchanged to `table_update`.

**Q: When CMD_CREATE_TABLE runs, why must `table_destroy` be called on the old table first?**
A: If there is already an active table, calling `table_create` would simply overwrite the pointer in `*active_table`. The old Table's heap memory (struct, name string, records array, all record name strings) would have no pointer pointing to it and could never be freed — a memory leak. `table_destroy` frees everything before the pointer is overwritten.

---

## 11. main.c

**File:** `src/main.c`

### Purpose
The entry point of the program. Implements the REPL (Read-Evaluate-Print Loop) — the interactive command-line shell that keeps the database alive and responsive.

### Dependencies

| Header | Why |
|---|---|
| `<stdio.h>` | For printf, fgets |
| `<stdlib.h>` | Included but not directly called (good practice for a main file) |
| `<string.h>` | For strcmp, strcspn, strlen |
| `"parser.h"` | To declare and use Command |
| `"table.h"` | To declare Table *active_table |
| `"executor.h"` | To call execute_command |
| `"storage.h"` | To call storage_save_table and storage_load_table |

### Key C Concepts

**`fgets(input_buffer, sizeof(input_buffer), stdin)`:** Reads up to `sizeof(input_buffer) - 1` characters from standard input (the keyboard) and stores them in the buffer, including the newline character. Returns NULL on end-of-file or error.

**`strcspn(input_buffer, "\n")`:** Returns the index of the first `\n` in the buffer. Setting that index to `'\0'` effectively removes the newline, since `\0` is the string terminator.

**`strcmp(str1, str2)`:** Returns 0 if the strings are equal. Used to check meta-commands.

**`while(1)`:** An infinite loop. The only way to exit is a `break` statement (triggered by `.exit`) or the program being terminated from outside.

### Memory Management Notes

- `active_table` starts as NULL. It is only non-NULL after a successful CREATE TABLE or .load.
- After the loop exits (via .exit), the final `if (active_table != NULL) { table_destroy(active_table); }` ensures all heap memory is freed before the program exits.
- The `.load` handler calls `table_destroy(active_table)` before loading, but does so before confirming the load will succeed. This is a known limitation: if `storage_load_table` then returns NULL, the old table is gone with no way to recover it.

### Viva Questions

**Q: What does `strcspn` do and why is it used to strip the newline?**
A: `strcspn(str, reject)` returns the length of the initial segment of `str` that contains no characters from `reject`. So `strcspn(input_buffer, "\n")` gives the index of the first newline. Setting `input_buffer[that_index] = '\0'` terminates the string there, effectively removing the newline. This is a standard C idiom for stripping trailing newlines.

**Q: Why is `active_table` declared as `Table *` and initialized to NULL?**
A: Starting as NULL makes the state explicit — no table exists yet. Every function that uses `active_table` checks for NULL first. If it were uninitialized, it would contain a garbage address, and checking for NULL would not reliably detect the "no table" state.

**Q: What happens if `fgets` returns NULL?**
A: This happens when the user presses Ctrl+D (end-of-file on Unix) or when there is a read error. The current code prints "Error reading input." and calls `continue` to try reading again. It does not exit the program, which means pressing Ctrl+D repeatedly will repeatedly print the error message without exiting. A better approach would be to break out of the loop on EOF.

**Q: Is there a memory leak if the user presses Ctrl+C instead of typing `.exit`?**
A: Yes. Ctrl+C sends SIGINT which immediately terminates the process without running any cleanup code. The OS reclaims the process's memory, so there is no OS-level leak, but the graceful `table_destroy` in main never runs. For a database, this means any unsaved data is lost. A production system would install a signal handler to catch SIGINT and call cleanup before exiting.

---

## 12. Makefile

**File:** `Makefile`

### Purpose
Defines the rules for compiling VaultDB. Running `make` compiles all source files into one executable. Running `make clean` removes the compiled output.

### Key Concepts

**Variables:** `CC`, `CFLAGS`, `CPPFLAGS`, `TARGET`, `SRC` are Makefile variables. They make the file easy to update — to change the compiler, you change one line.

**`$(TARGET): $(SRC)`** — The `vaultdb` binary depends on all source files. If any `.c` file changes, `make` will recompile.

**Tab requirement:** The recipe line `$(CC) $(CFLAGS) ...` must start with a literal Tab character, not spaces. This is a quirk of the original Makefile format.

**CPPFLAGS vs CFLAGS:** `-Iinclude` belongs in `CPPFLAGS` (C preprocessor flags) because it is used during the preprocessing stage when `#include` directives are resolved. `-Wall`, `-std=c11`, etc. belong in `CFLAGS` because they control the compiler itself.

