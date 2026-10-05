# Document 1: VaultDB — Project Overview

## Table of Contents
1. [What is VaultDB?](#1-what-is-vaultdb)
2. [Project Folder Structure](#2-project-folder-structure)
3. [Build Process](#3-build-process)
4. [How the Program Runs — Step by Step](#4-how-the-program-runs--step-by-step)
5. [Data Flow Diagram](#5-data-flow-diagram)
6. [Supported Commands](#6-supported-commands)

---

## 1. What is VaultDB?

VaultDB is a lightweight, command-line database engine built entirely from scratch in C11. It has no dependency on SQLite, MySQL, or any other external database library. Everything — the data structures, the query parser, the binary file storage, and the interactive shell — was written by hand.

It supports a small but complete set of database operations:

- Creating a table in memory.
- Inserting, reading, updating, and deleting records.
- Saving the entire table to a binary file on disk.
- Loading that file back into memory on the next run.
- Accepting live queries through an interactive command-line REPL (Read-Evaluate-Print Loop).

VaultDB is intentionally minimal. It holds exactly one table in memory at a time. That table holds records of a fixed shape: an integer ID, a string name, an integer price, and an integer stock count.

---

## 2. Project Folder Structure

```
VaultDB/
├── Makefile                  Build instructions for gcc
├── README.md                 Short project description
├── PROJECT_GUIDE.md          Development roadmap used during construction
├── data/                     Folder where .db binary files are saved
│   └── vault.db              The binary database file created at runtime
├── include/                  Header files (declarations only)
│   ├── record.h              Declares the Record struct and its two functions
│   ├── table.h               Declares the Table struct and all CRUD functions
│   ├── storage.h             Declares the two file I/O functions
│   ├── parser.h              Declares the Command struct, CommandType enum, and parse_command
│   └── executor.h            Declares execute_command
├── src/                      Source files (all actual logic lives here)
│   ├── main.c                Entry point; runs the REPL loop
│   ├── record.c              Implements record_create and record_destroy
│   ├── table.c               Implements the dynamic array and all CRUD functions
│   ├── storage.c             Implements binary save and load
│   ├── parser.c              Implements parse_command using sscanf
│   └── executor.c            Implements execute_command; bridges parser and table
└── tests/                    Reserved for future unit tests (currently empty)
```

### Why separate include/ and src/?

This is the standard layout for C projects. Header files (.h) contain only declarations — they tell the compiler "this function exists and here is its signature." Source files (.c) contain the definitions — the actual code. Separating them lets any .c file include any .h file to access the declarations it needs, without copying code around.

---

## 3. Build Process

### The Compile Command

Running `make` executes this single command:

```
gcc -Wall -Wextra -Wpedantic -std=c11 -Iinclude \
    src/main.c src/record.c src/table.c src/storage.c src/parser.c src/executor.c \
    -o vaultdb
```

### Compiler Flags Explained

| Flag | What it does | Why it is used |
|---|---|---|
| -Wall | Enables all common warnings | Catches mistakes that are legal but almost certainly wrong |
| -Wextra | Enables additional warnings not in -Wall | Catches more subtle issues |
| -Wpedantic | Enforces strict ISO C compliance | Ensures code is portable and standard |
| -std=c11 | Compiles as C11 (2011 standard) | Allows modern features like loop-scoped int declarations |
| -Iinclude | Adds include/ to header search path | Allows #include "record.h" instead of #include "../include/record.h" |

### make clean

Deletes the compiled vaultdb binary and any .o object files. Run before committing to avoid tracking binaries in git.

---

## 4. How the Program Runs — Step by Step

1. Program starts. OS calls main() in src/main.c.
2. Initialization. main() declares Table *active_table = NULL.
3. REPL loop begins. while(1) starts. Prints vaultdb> and waits.
4. Read input. fgets() reads the user's line. strcspn strips the trailing newline.
5. Check meta-commands. .exit breaks, .save calls storage_save_table(), .load calls storage_load_table().
6. Parse SQL query. The string is passed to parse_command() in parser.c. sscanf matches patterns and returns a filled Command struct.
7. Execute. The Command struct is passed to execute_command() in executor.c. A switch statement dispatches to the correct table.c function.
8. Table operations. table.c modifies the dynamic array of Record structs, growing it with realloc if needed.
9. Result printed. executor.c prints "Inserted 1 row." or the formatted SELECT output.
10. Loop repeats.
11. Shutdown. After .exit, table_destroy() frees all memory. Program exits with return 0.

---

## 5. Data Flow Diagram

```
User types a query
        |
        v
   main.c (main)
   |-- Meta-command? (.save / .load / .exit)
   |         |
   |         v
   |    storage.c <----> data/vault.db (binary file on disk)
   |
   +-- SQL query?
             |
             v
        parser.c (parse_command)
        Uses sscanf to extract fields
        Returns: Command struct
             |
             v
        executor.c (execute_command)
        switch on cmd.type
             |
             v
        table.c + record.c
        (in-memory dynamic array)
             |
             v
        printf() -> Terminal output
```

---

## 6. Supported Commands

All commands are case-sensitive. The semicolon ; at the end is required.

| Command | Syntax | What it does |
|---|---|---|
| CREATE TABLE | CREATE TABLE products; | Creates a new empty table named products |
| INSERT | INSERT INTO products VALUES (1, "Keyboard", 1200, 10); | Inserts a record |
| SELECT | SELECT * FROM products; | Prints all records |
| UPDATE (price) | UPDATE products SET price = 999 WHERE id = 1; | Updates price of id=1 |
| UPDATE (stock) | UPDATE products SET stock = 5 WHERE id = 1; | Updates stock of id=1 |
| DELETE | DELETE FROM products WHERE id = 1; | Deletes record with id=1 |
| .save | .save | Saves table to data/vault.db |
| .load | .load | Loads table from data/vault.db |
| .exit | .exit | Frees memory and exits |

Note: The parser is case-sensitive. UPDATE only supports setting price or stock. SELECT has no WHERE clause support.
