# Document 4: Viva Preparation

## Table of Contents
1. [C Fundamentals](#1-c-fundamentals)
2. [Pointers and Memory](#2-pointers-and-memory)
3. [Data Structures](#3-data-structures)
4. [File I/O and Persistence](#4-file-io-and-persistence)
5. [Parsing](#5-parsing)
6. [Program Architecture](#6-program-architecture)
7. [Build Process](#7-build-process)
8. [Likely Tricky Questions](#8-likely-tricky-questions)

---

## 1. C Fundamentals

**Q1: What is the difference between `.` and `->` when accessing a struct field?**
A: Use `.` when you have the struct value directly (e.g., `Record r; r.id`). Use `->` when you have a pointer to a struct (e.g., `Record *r; r->id`). The arrow is shorthand for dereferencing then accessing: `r->id` is exactly the same as `(*r).id`.

**Q2: What is a `typedef` and why is it used for structs in this project?**
A: `typedef` creates an alias for a type name. Without it, you would have to write `struct Record *r` every time. With `typedef struct Record {...} Record;`, you can write just `Record *r`. It reduces verbosity and is the standard C style for structs used as public API types.

**Q3: What is an enum and what does the compiler do with it?**
A: An enum is a named set of integer constants. `CMD_UNKNOWN = 0`, `CMD_CREATE_TABLE = 1`, etc. The compiler replaces the names with integers. Enums make code readable — `switch (cmd.type)` with `case CMD_INSERT:` is far clearer than `switch (cmd.type)` with `case 2:`.

**Q4: What does `static` mean when applied to a function?**
A: At file scope, `static` limits the function's visibility to the file it is defined in. `static char *read_string(FILE *file)` in `storage.c` cannot be called from `table.c` or `main.c`. It is a private helper function. This prevents accidental misuse and keeps the global namespace clean.

**Q5: What is the difference between `const char *name` and `char *const name`?**
A: `const char *name` means the characters pointed to are read-only (the pointer itself can be changed). `char *const name` means the pointer is read-only (but the characters can be changed). In `record_create(const char *name)`, it means: "I will not modify the string you pass me." This is the correct and safer choice.

**Q6: What is `sizeof` and when is it evaluated?**
A: `sizeof` is a compile-time operator that returns the size in bytes of a type or variable. `sizeof(Record)` is computed by the compiler and replaced with a constant integer in the machine code. It is NOT a function call and has no runtime cost.

---

## 2. Pointers and Memory

**Q7: What is the heap and how does it differ from the stack?**
A: The stack holds local variables and function call frames. It is automatically managed — memory is allocated when a function is called and freed when it returns. The heap is a large pool of memory managed manually by the programmer using `malloc` and `free`. Heap memory persists until `free` is called, regardless of which function allocated it. In VaultDB, all Table and Record data lives on the heap so it can outlast the function that created it.

**Q8: What does `malloc` return and what must you always do with it?**
A: `malloc` returns a `void *` (generic pointer) to the start of a newly allocated block of at least the requested size, or `NULL` if allocation failed. You must always check the return value. Dereferencing a NULL pointer is undefined behaviour and typically causes a segfault.

**Q9: What is a memory leak and does VaultDB have any?**
A: A memory leak occurs when heap memory is allocated but never freed, even though the program no longer needs it and has no pointer to reach it. In VaultDB, the main known risk is the `.load` handler, which destroys the old table before confirming the new one loaded successfully. If the load fails, the old table is leaked. All other paths correctly free memory before losing pointers.

**Q10: What is a dangling pointer?**
A: A dangling pointer is a pointer that holds the address of memory that has already been freed. Reading or writing through a dangling pointer is undefined behaviour. In VaultDB, `table_select` returns a pointer into the live records array. If a `table_insert` after that call triggers `realloc` and moves the array, the pointer from `table_select` becomes dangling.

**Q11: What is the difference between `free(ptr)` and `ptr = NULL`?**
A: `free(ptr)` releases the heap memory at the address in `ptr`. The pointer variable itself still holds the old address afterward — it is now dangling. Setting `ptr = NULL` after freeing is a defensive practice that makes the dangling pointer safe: reading NULL and checking for it catches bugs early.

**Q12: What is a segmentation fault?**
A: A segfault occurs when a program attempts to access memory it does not own — typically by dereferencing NULL, writing to freed memory, or writing past the end of an array. The OS detects this and terminates the program. In VaultDB's early development, writing `strcpy(table->name, name)` before mallocing memory for `table->name` caused a segfault because `table->name` was an uninitialized garbage pointer.

**Q13: Why is `Table **active_table` used in `execute_command`?**
A: `execute_command` must be able to change what `active_table` points to (e.g., to create a new table). In C, if you pass `Table *active_table`, you can modify the Table the pointer points to, but you cannot change which Table the caller's pointer points at. By passing `Table **active_table` (a pointer to the pointer), the function can write `*active_table = table_create(...)` to change the caller's variable.

**Q14: Explain the realloc safety pattern used in `table_insert`.**
A: Writing `table->records = realloc(table->records, new_size)` is dangerous. If `realloc` fails and returns NULL, you have just overwritten your only pointer to the old array. That memory is now leaked and unreachable. The safe pattern is:
```c
Record *temp = realloc(table->records, new_size);
if (temp == NULL) return -1;   // old data still safe
table->records = temp;
```

---

## 3. Data Structures

**Q15: How does VaultDB's dynamic array work?**
A: The Table struct holds three pieces of information about its records: `records` (a pointer to a contiguous block of Record structs), `record_count` (how many are filled), and `capacity` (how many slots exist). When `record_count == capacity`, `table_insert` calls `realloc` to double the capacity. The data is preserved by `realloc`, and new records fill the expanded space.

**Q16: Why does VaultDB double the capacity on each growth, rather than adding a fixed amount?**
A: Doubling gives amortized O(1) insertion. If you always grow by 1, every insert triggers a realloc (which copies all existing data), giving O(n) per insert and O(n²) total for n inserts. With doubling, reallocs happen at insert counts 1, 2, 4, 8, 16... — logarithmically rarely. The total copy work is O(n), making each insert amortized O(1).

**Q17: What is linear search and what is its time complexity? Why does VaultDB use it?**
A: Linear search scans the array from start to finish until it finds a match. Its worst-case time complexity is O(n) — proportional to the number of records. VaultDB uses it because the implementation is simple and correct, and for small record counts (which VaultDB is designed for), it is fast enough. A production database would use a B-tree index for O(log n) lookups.

**Q18: In `table_delete`, how is the gap after a deleted record closed?**
A: The records after the deleted index are shifted left by one position using a loop: `records[j] = records[j+1]` for j from the deleted index to `record_count - 2`. This overwrites the deleted slot and fills the gap. `record_count` is then decremented.

**Q19: Why does VaultDB store Records by value in the array, not as pointers?**
A: Storing by value (`Record *records` where `records` is an array of Record structs) keeps all records in a single contiguous block of memory. This is cache-friendly and requires only one free for the entire array. Storing as pointers (`Record **records`) would require n separate mallocs and frees for n records, and the memory would be scattered, causing more cache misses.

---

## 4. File I/O and Persistence

**Q20: What is the difference between text mode and binary mode for files?**
A: In text mode (`"r"`, `"w"`), the C library may translate newline characters. On Windows, `\n` is translated to `\r\n` on write and back on read. In binary mode (`"rb"`, `"wb"`), bytes are written and read exactly as they are in memory, with no translation. For VaultDB's raw integer and string bytes, binary mode is required to prevent corruption.

**Q21: How does VaultDB know where one field ends and the next begins in the binary file?**
A: Integers always occupy exactly `sizeof(int)` bytes (typically 4). For strings, VaultDB writes the length as an integer first, then the bytes of the string. The reader reads the length first, allocates a buffer of that size, then reads exactly that many bytes. This length-prefix encoding is a standard technique for binary formats.

**Q22: What does `fwrite` return and is its return value checked in VaultDB?**
A: `fwrite` returns the number of items successfully written. If a disk error occurs, it may return less than requested. VaultDB's `storage_save_table` does not check the return values of its `fwrite` calls. This is a correctness gap — a partial write would produce a corrupted file. `storage_load_table` correctly checks `fread` return values and uses the `goto fail` pattern to handle errors.

**Q23: What does `fopen` return if the file does not exist when opened in "rb" mode?**
A: It returns NULL. The code checks for this and returns NULL from `storage_load_table`. This is the correct and expected behaviour when the user types `.load` before ever saving.

**Q24: What is the `goto fail` pattern and why is it appropriate here?**
A: The load function has a long sequence of steps, each of which can fail. On failure, the same three cleanup actions are always needed: close the file, destroy the partially built table, return NULL. Using `goto fail` to jump to a single cleanup label avoids duplicating those three lines at every failure point. This is the standard C cleanup idiom, used throughout the Linux kernel.

---

## 5. Parsing

**Q25: What does `sscanf` do?**
A: `sscanf(string, format, ...)` is the inverse of `printf`. It reads formatted data from a string rather than printing formatted data. It attempts to match the string against the format pattern and extracts values into the provided pointer arguments. It returns the number of items successfully matched and assigned.

**Q26: In the INSERT sscanf format string, what does `%255[^\"]` mean?**
A: This is a character class format specifier. `[^\"]` means "match any character that is not a double-quote." The `255` limits the match to 255 characters to avoid overflowing the 256-byte buffer. The surrounding `\"` in the format string match the literal double-quote characters that the user types around the record name.

**Q27: Why are some sscanf arguments passed with `&` and some without?**
A: `sscanf` needs pointers to write into. For integer variables like `cmd.id`, you write `&cmd.id` to pass the address. For char arrays like `cmd.table_name`, the array name already decays to a pointer to its first element, so `&` is not needed (and would be wrong).

**Q28: Why is VaultDB's parser case-sensitive?**
A: `sscanf` does exact byte-by-byte matching. The format string `"INSERT INTO"` only matches the uppercase sequence. Adding case-insensitivity would require converting the input string to uppercase/lowercase before parsing, which is not implemented.

**Q29: Could VaultDB's parser be fooled by valid-looking but incorrect input?**
A: Yes. `sscanf` with `%255s` stops at whitespace, so `INSERT INTO some table` would parse "some" as the table name and fail. But `%255[^;]` (used for CREATE TABLE and SELECT) stops at `;`, so unusual table names with spaces might partially work or fail silently. The parser is MVP-level: it works for the exact supported syntax and nothing else.

---

## 6. Program Architecture

**Q30: Why is VaultDB split into multiple .c and .h files instead of one big file?**
A: Separation of concerns. Each file has a single responsibility: record.c manages records, table.c manages the dynamic array, storage.c handles disk I/O, parser.c handles string parsing, executor.c bridges them. This makes each part easier to understand, test, and modify without affecting the others. It also allows the compiler to only recompile changed files in a larger project.

**Q31: What is the REPL pattern and why is it used?**
A: REPL stands for Read-Evaluate-Print Loop. The program reads one line of input, evaluates it (parses then executes), prints the result, and loops. This is the standard pattern for interactive command-line programs and shells. It is simple and immediately responsive to user input.

**Q32: Why does VaultDB only support one table at a time?**
A: MVP scope decision. Supporting multiple tables would require a way to name and switch between tables, a different data model (a database containing a collection of tables), and more complex storage format. These are non-trivial to implement correctly and were deferred as future work.

**Q33: Trace what happens when the user types `INSERT INTO products VALUES (1, "Mouse", 500, 10);`**
A: 
1. `main.c` reads the string via `fgets`, strips the newline.
2. `parse_command` is called. The INSERT sscanf matches all 5 fields: table_name="products", id=1, record_name="Mouse", price=500, stock=10. Returns a Command with type=CMD_INSERT.
3. `execute_command` is called with that Command and `&active_table`.
4. The switch hits `case CMD_INSERT`. Checks `*active_table != NULL`.
5. `table_insert(*active_table, 1, "Mouse", 500, 10)` is called.
6. If the array has capacity, a slot is taken. Name is malloced and copied. Fields assigned. record_count++. Returns 0.
7. executor prints "Inserted 1 row."

---

## 7. Build Process

**Q34: What does the `-Iinclude` flag do?**
A: It tells the C preprocessor to look in the `include/` directory when resolving `#include "..."` directives. Without it, the compiler would only look in the same directory as the source file being compiled, and `#include "record.h"` in a file inside `src/` would fail to find `include/record.h`.

**Q35: What is the difference between a compiler warning and a compiler error?**
A: An error prevents compilation — the code cannot be turned into a program. A warning means the code compiled but the compiler suspects something is wrong (e.g., uninitialized variable, missing return). VaultDB uses `-Wall -Wextra -Wpedantic` to catch as many warnings as possible. The project rule is zero warnings.

**Q36: What does `make clean` do and why is it important before committing to git?**
A: `make clean` runs `rm -f vaultdb *.o`, deleting the compiled binary and object files. These are platform-specific binary artifacts that should not be tracked in git — they are large, change every compile, and are not portable between operating systems or machines. The `.gitignore` file excludes them as well.

---

## 8. Likely Tricky Questions

**Q37: Why does `parse_command` return `Command` by value, not `Command *`?**
A: Returning a pointer to a local variable is undefined behaviour in C. The local `Command cmd` lives on the stack frame of `parse_command`. When the function returns, that stack frame is destroyed, and the pointer would be dangling — pointing to memory that may be overwritten at any moment. Returning by value copies the struct to the caller's stack, which is safe. The Command struct only contains ints and fixed char arrays (no heap allocations), so copying it is correct and cheap.

**Q38: Why is `Table **` needed in `execute_command` instead of just `Table *`?**
A: The executor needs to be able to replace the caller's `Table *active_table` with a new pointer (e.g., for CMD_CREATE_TABLE). In C, function arguments are passed by value — the function gets a copy. If you pass `Table *`, the function can modify the Table's fields but cannot change which Table the caller's pointer points to. By passing `Table **` (the address of the pointer), the function can write `*active_table = table_create(...)` to change the caller's variable directly.

**Q39: Why does `storage_load_table` use `goto fail` instead of just returning NULL directly?**
A: By the time a failure occurs inside the load loop, two resources may be open: the file handle and the partially constructed Table. Both need to be cleaned up before returning. Without goto, you would have to write `fclose(file); table_destroy(table); return NULL;` at every single failure point — there are five such points in the current code. The goto jumps to one place that does all three cleanup actions. It is cleaner, less error-prone, and is the standard C pattern for this.

**Q40: Why does VaultDB use `sscanf` instead of writing a proper tokenizer?**
A: VaultDB supports only 5 fixed query patterns with rigid syntax. Writing a proper tokenizer (which handles arbitrary whitespace, case-insensitivity, optional clauses, nested expressions, etc.) would be hundreds of lines of code and significant complexity. For this MVP scope, `sscanf` achieves 95% of the result in 5 lines of code per command. The trade-off is that the parser is brittle — any deviation from the exact expected format fails silently.

**Q41: What would happen if you called `table_destroy` twice on the same Table?**
A: Double-free. The first call frees all the memory. The second call tries to free memory that is no longer owned by the program. This is undefined behaviour and typically causes a crash or heap corruption. The code prevents this by always setting the pointer to NULL after destroying: `table_destroy(active_table); active_table = NULL;` — though looking at `main.c`, the code does NOT actually set `active_table = NULL` after calling `table_destroy` inside the `.load` handler. This could be a problem if `storage_load_table` then returns NULL and some later path tries to destroy it again.

**Q42: What is the SELECT output's format specifier `%-10s` doing?**
A: `%` starts a format spec. `-` means left-align. `10` is the minimum field width — if the string is shorter than 10 characters, it is padded with spaces on the right to make it 10 characters wide. `s` means string. This creates a neat columnar output where the Name column is always exactly 10 characters wide, regardless of the actual name length.

**Q43: In `table_delete`, why must `records[i].name` be freed before the shift loop, not after?**
A: The shift loop copies `records[j+1]` over `records[j]`. If we freed `records[i].name` after the shift, we would have already overwritten slot i with the data from slot i+1. Specifically, `records[i].name` would now point to the same address as `records[i+1].name` (since the struct was copied). Freeing it would free the name of what is now records[i] (formerly records[i+1]), causing a double-free when table_destroy later frees it again.

**Q44: What is `strcspn` and how is it used to remove the newline from user input?**
A: `strcspn(str, reject)` returns the number of characters at the start of `str` before any character in `reject` is found. `strcspn(input_buffer, "\n")` returns the index of the first newline character. Setting `input_buffer[that_index] = '\0'` terminates the string there, effectively removing the newline. This is the standard C idiom for stripping trailing newlines from `fgets` output.

**Q45: If VaultDB's binary file is opened in a hex editor, what would you see for a table named "sam" with one record (id=1, name="Alice", price=100, stock=10)?**
A: (Approximate, using 4-byte little-endian integers on a typical Mac)
- Bytes 0-3: `04 00 00 00` (name length = 4, including null terminator)
- Bytes 4-7: `73 61 6D 00` ("sam\0")
- Bytes 8-11: `01 00 00 00` (record_count = 1)
- Bytes 12-15: `01 00 00 00` (record id = 1)
- Bytes 16-19: `64 00 00 00` (price = 100)
- Bytes 20-23: `0A 00 00 00` (stock = 10)
- Bytes 24-27: `06 00 00 00` (name length = 6, "Alice\0")
- Bytes 28-33: `41 6C 69 63 65 00` ("Alice\0")
