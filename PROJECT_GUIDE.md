# VaultDB Project Guide

This guide is your master plan for building VaultDB. It's written specifically for where you are right now—knowing C syntax, but ready to learn how systems programming, memory management, and project architecture actually work.

---

## 1. PROJECT OVERVIEW

**What is VaultDB?**
VaultDB is a lightweight, offline-first embedded database engine built entirely from scratch in C. 

**Why does it exist?**
Imagine a small local shop, clinic, or personal app that needs to save data (like a list of products or customers) so it doesn't disappear when the computer turns off. Instead of installing a massive database server like MySQL or PostgreSQL, which requires networking, passwords, and background services, they can use VaultDB. It simply saves data to a local file (e.g., `shop.db`).

**Why is it an "engine" and not a "CRUD app"?**
A CRUD (Create, Read, Update, Delete) app is built for *one specific purpose*—like an inventory app that only knows about "products." An **engine** is generic. VaultDB doesn't know what a "product" or a "customer" is. It only knows how to store *tables* and *records*, parse commands, and read/write to a file. It is the underlying machinery that *powers* a CRUD app.

---

## 2. CORE CONCEPTS I NEED TO UNDERSTAND FIRST

Before writing the rest of the database, you need to understand the underlying mechanics of C that make it possible.

### Pointers and Structs
A pointer is simply a variable that holds a memory address. Why do we use a pointer like `char* name` inside a `struct` instead of a fixed array like `char name[50]`?
Because fixed arrays waste space (if a name is 3 letters, you waste 47 bytes) and artificially limit data (what if a name is 55 letters?). A `char*` pointer lets us dynamically allocate exactly as much memory as we need for the string at runtime.

```c
struct Record {
    int id;
    char *name; // Points to an exact amount of memory we will allocate later
    int price;
};
```

### Dynamic Memory and Ownership
In C, when you use `malloc()` (memory allocate), you ask the operating system for a chunk of memory on the "heap" (a big pool of available memory). This memory stays yours until you explicitly give it back using `free()`.

**Ownership** means deciding *whose job it is* to call `free()`. If a `Table` "owns" a `Record`, the table is responsible for freeing the record. If you forget to free memory, you get a **memory leak** (your program slowly eats all RAM). If you free it twice, you get a **double free** crash. If you use it after freeing it, you get a **dangling pointer** crash.

```c
// Allocating exact memory for a name
char *name = malloc(10); // Ask for 10 bytes
strcpy(name, "Keyboard"); // Use it
free(name); // Give it back!
```

### Struct Composition
Databases are hierarchical. You build them by putting structs inside other structs.
- A `Record` represents one row.
- A `Table` represents a collection of rows, so it contains an array of `Record`s.
- A `Database` contains an array of `Table`s.

```c
struct Table {
    char *name;
    struct Record *records; // A pointer to an array of Records
    int record_count;
};
```

### Header Files (.h) vs Source Files (.c)
- **Header Files (.h)**: These contain *declarations*. They are the "menu" of your code. They tell other files what structs and functions exist, but don't contain the actual code.
- **Source Files (.c)**: These contain the *implementation*. They are the "kitchen." This is where the actual code logic lives.
We separate them so that multiple `.c` files can use the same `.h` menu without duplicating the kitchen.

### How a Makefile Works
When you type `make`, it looks for a file named `Makefile`. A Makefile is just a script that tells the compiler (like `gcc`) exactly how to glue all your `.c` files together into a final runnable program. It saves you from typing `gcc main.c record.c table.c -o vaultdb -Wall -Wextra ...` every single time you change a line of code.

### File I/O in C
To save data so it survives when the program closes, we write to a file on the hard drive.
- `fopen()`: Opens a file.
- `fwrite()`: Writes raw bytes from your structs into the file.
- `fread()`: Reads raw bytes from the file back into your structs.
- `fclose()`: Closes the file and saves it safely.

```c
FILE *file = fopen("database.db", "wb"); // Open in write-binary mode
int number = 42;
fwrite(&number, sizeof(int), 1, file); // Write the integer to disk
fclose(file);
```

### Tokenizer / Parser
If a user types `SELECT * FROM products;`, the computer just sees a long string of characters. 
- A **Tokenizer** breaks the string into logical words (tokens): `["SELECT", "*", "FROM", "products", ";"]`.
- A **Parser** looks at those tokens and figures out the *meaning*: "The user wants to read all records from the table named products." It turns text into a command struct the code can execute.

---

## 3. ARCHITECTURE

VaultDB is designed as a pipeline. Each layer has **one single responsibility**.

**User → CLI → Parser → Executor → Storage Engine / Index Manager → database.db**

1. **CLI (Command Line Interface)**: Gets text input from the user and prints results to the screen. It knows nothing about how data is saved.
2. **Parser**: Takes the user's text, tokenizes it, and figures out what command they want to run. It does not actually run the command.
3. **Executor**: Takes the parsed command and performs the actual logic (e.g., finding the right table, adding the record).
4. **Storage Engine**: Handles the dirty work of converting C structs into raw bytes and saving/reading them from `database.db` on the hard drive.
5. **Index Manager**: (Built later) A fast lookup system so we don't have to check every single row to find ID #500.

---

## 4. REPOSITORY / FILE STRUCTURE

Here is exactly how the repository is structured, with a purpose for every file:

```text
vaultdb_c_uca/
├── src/
│   ├── main.c        (The entry point that starts the program and CLI)
│   ├── record.c      (Implementation of record creation/destruction logic)
│   ├── table.c       (Implementation of table logic, inserting rows, growing arrays)
│   ├── storage.c     (Implementation of reading/writing to the .db file)
│   ├── parser.c      (Implementation of turning text into commands)
│   ├── executor.c    (Implementation of executing parsed commands)
│   ├── index.c       (Implementation of the fast lookup system)
│   └── utils.c       (Helper functions used across multiple files)
├── include/
│   ├── record.h      (Declarations for the Record struct and its functions)
│   ├── table.h       (Declarations for the Table struct and its functions)
│   ├── storage.h     (Declarations for file storage functions)
│   ├── parser.h      (Declarations for the parser and command types)
│   ├── executor.h    (Declarations for the execution engine)
│   ├── index.h       (Declarations for indexing functions)
│   └── utils.h       (Declarations for shared helpers)
├── tests/            (Small driver programs to test our C code independently)
├── data/             (Where our actual database.db files will be saved during dev)
├── README.md         (The final project documentation and proposal)
├── Makefile          (The script that compiles the project)
├── LICENSE           (The open-source license for the code)
└── .gitignore        (Tells Git to ignore compiled files like .o and .db files)
```

---

## 5. DATA MODEL

### Record
```c
struct Record {
    int id;
    char *name;   // Dynamically allocated!
    int price;
    int stock;
};
```

### Table
```c
struct Table {
    char *name;
    struct Record *records; // Array of records
    int record_count;       // How many records actually exist right now
    int capacity;           // How much memory is allocated for the array
};
```

**Memory Layout & Array Growth**
Imagine `capacity` is the number of chairs in a room, and `record_count` is the number of people sitting down.
```text
Capacity: 4
Count: 2
[ Record 1 ] [ Record 2 ] [ Empty Chair ] [ Empty Chair ]
```
When a 5th person arrives, we don't have space. We use `realloc()` to move to a bigger room (double the capacity to 8), copy everyone over, and sit the 5th person down.

**Ownership Rules (CRITICAL):**
- **Record**: When `record_create` is called, it allocates memory for the `name`. When `record_destroy` is called, it MUST free the `name` memory before freeing the struct itself.
- **Table**: The Table owns its `records` array. When `table_destroy` is called, it must loop through every record in the array and call `record_destroy` on it, and ONLY THEN free the `records` array, and finally free its own `name` and struct.

---

## 6. STORAGE DESIGN

We write data to a file because RAM is volatile and is cleared when a program stops. By writing bytes to the hard drive, data **persists**.

**The Lifecycle:**
1. **Open**: Use `fopen` on `database.db`. If it doesn't exist, create it.
2. **Read**: Read the header (how many tables, how many records) so the program knows what to load into memory.
3. **Operate**: User runs queries in memory (much faster than writing to disk every second).
4. **Write**: When the user inserts or updates, write those specific byte changes back to the file.
5. **Close**: Safely `fclose` the file before exiting so the OS flushes changes to the disk.

---

## 7. QUERY LANGUAGE

We will support a small, custom SQL-like subset.

**Supported Grammar:**
- `CREATE TABLE <name> (id, name, price, stock);`
- `INSERT INTO <table> VALUES (<id>, "<name>", <price>, <stock>);`
- `SELECT * FROM <table>;`
- `SELECT * FROM <table> WHERE <column> = <value>;`
- `UPDATE <table> SET <column> = <value> WHERE <column> = <value>;`
- `DELETE FROM <table> WHERE <column> = <value>;`

*Example:*
`INSERT INTO products VALUES (1, "Keyboard", 1200, 10);`

---

## 8. STEP-BY-STEP IMPLEMENTATION ROADMAP

Do not write everything at once. We will build this in tiny, ~100-150 line phases. 

### Phase 1: Table Insertion & Array Growth
- **Concepts needed:** Dynamic memory (`realloc`), pointers to arrays.
- **What to build:** Write `table_insert` in `table.c`. Check if `record_count == capacity`. If yes, double the capacity using `realloc`. Then add the record to the array and increment `record_count`.
- **Done looks like:** A test in `main.c` that inserts 5 records into a table with an initial capacity of 2, proving the capacity automatically grows to 4, then 8, without crashing.
- **Git:** `git commit -m "Implement table insertion and dynamic array growth"`

### Phase 2: In-Memory CRUD (Read/Update/Delete)
- **Concepts needed:** Loops, array shifting.
- **What to build:** Write functions to find a record by ID, update its values, and delete a record (by shifting all subsequent records to the left to fill the gap).
- **Done looks like:** A test in `main.c` that inserts a record, updates its price, asserts the price changed, deletes it, and asserts `record_count` went down by 1.
- **Git:** `git commit -m "Implement in-memory read, update, and delete"`

### Phase 3: Persistence (File I/O)
- **Concepts needed:** `fopen`, `fwrite`, `fread`.
- **What to build:** In `storage.c`, write a function to save the entire table and its records to a file, and another function to load it from the file back into a `Table` struct.
- **Done looks like:** A test that creates a table, inserts a record, saves it to disk, destroys the in-memory table, then loads it back from disk and successfully prints the record.
- **Git:** `git commit -m "Implement basic file persistence for tables and records"`

### Phase 4: Tokenizer & Parser
- **Concepts needed:** String manipulation (`strtok`, `strcmp`), struct composition.
- **What to build:** In `parser.c`, write a function that takes a string like `INSERT INTO products VALUES (1, "A", 10, 5)` and breaks it into tokens, then populates a `Command` struct specifying it's an INSERT command with those specific values.
- **Done looks like:** A test that passes a raw string to the parser, and prints out the resulting `Command` struct's type and values.
- **Git:** `git commit -m "Implement query tokenizer and basic parser"`

### Phase 5: The Executor
- **Concepts needed:** Architecture separation.
- **What to build:** In `executor.c`, write a function that takes the `Command` struct from Phase 4, looks at the command type (e.g., INSERT), and calls the actual `table_insert` function we wrote in Phase 1.
- **Done looks like:** You pass a string query to the parser, pass the result to the executor, and the table updates accordingly.
- **Git:** `git commit -m "Implement executor to bridge parser and table logic"`

### Phase 6: WHERE Filtering & Search
- **Concepts needed:** Linear search algorithm.
- **What to build:** Update the parser to understand `WHERE id = X`. Update the executor to loop through the table (linear search, O(n) time) and only return/update/delete the record that matches the ID.
- **Done looks like:** Running an `UPDATE ... WHERE id = 2` query and proving only record #2 changes.
- **Git:** `git commit -m "Implement WHERE clause and linear search"`

### Phase 7: Interactive CLI
- **Concepts needed:** Infinite loops, standard input (`fgets`).
- **What to build:** In `main.c`, create an infinite `while(1)` loop that prints `vaultdb> `, waits for the user to type a string, passes it to the parser -> executor, prints the result, and loops again until they type `exit`.
- **Done looks like:** A fully working interactive prompt in the terminal.
- **Git:** `git commit -m "Add interactive CLI loop"`

*(Further phases for Indexing, Testing, and Performance will follow once the core engine is working).*

---

## 9. MY CURRENT STATE

- [x] Repo scaffolding, `README.md`, `Makefile`, `LICENSE`, `.gitignore` exist.
- [x] Git initialized with multiple commits.
- [x] Strict C11 compilation (`-Wall -Wextra -Wpedantic -std=c11`) configured.
- [x] `src/main.c` exists.
- [x] `include/record.h` and `src/record.c` implemented and tested (create/destroy work correctly).
- [x] `include/table.h` and `src/table.c` implemented and tested (create/destroy work correctly).
- [x] Makefile builds `main.c`, `record.c`, `table.c` with zero warnings.

**NEXT STEP:** Start Phase 1 (Table Insertion & Array Growth).

---

## 10. TESTING STRATEGY

Since we aren't using a massive C testing framework, a "test" simply means writing a short function in `main.c` (or a dedicated file in `tests/`) that runs code and checks if the result matches what we expect. 

- **Functional Tests:** Normal use cases. (e.g., Test that inserting 1 record makes `record_count` equal 1).
- **Edge-case Tests:** Weird or extreme scenarios. (e.g., Test what happens if we pass a negative price, or insert a string that is 1000 characters long, or try to delete an ID that doesn't exist).
- **Integration Tests:** Testing the whole pipeline at once. (e.g., Type a query into the CLI, let it parse, execute, save to disk, and verify the file grew in size).

---

## 11. CODE QUALITY & GIT RULES

- **Compilation:** Your code must compile with `make` using `-Wall -Wextra -Wpedantic -std=c11` with **ZERO warnings**. Warnings are errors in disguise.
- **Commits:** Do not write code for 3 days and run `git commit -m "did stuff"`. Commit at the end of every tiny phase listed in Section 8. Commits should explain *what* you changed.
- **Memory Check:** Every time you use `malloc`, check if it returns `NULL`. Every time you allocate, you must `free`.
- **Formatting:** Keep indentation consistent.

---

## 12. FINAL README.md REQUIREMENTS

When the code is done, your `README.md` (which serves as your final project proposal) must contain these exact sections:
1. Title/description
2. Problem statement
3. Motivation
4. Goals
5. Functional specifications
6. Non-functional specifications
7. Architecture
8. Module structure
9. Data structures
10. Storage design
11. Query syntax
12. Algorithms
13. Complexity
14. Installation
15. Compilation
16. Usage
17. Testing
18. Performance
19. Limitations
20. Future work

---

## 13. VIVA / INTERVIEW PREP

You need to defend this code. Here are model answers so you understand *why* you are building it this way.

**What exactly is VaultDB?**
It is a custom, lightweight embedded database engine built in C that stores records in local files and supports simple SQL-like queries.

**Why is it a database engine rather than a CRUD application?**
Because it doesn't know about specific business logic (like products or users); it provides generic infrastructure (tables, parsers, storage) that any application can use.

**Why C?**
C forces direct memory management and file I/O handling, which is essential for understanding how actual database systems work under the hood.

**What is a record?**
A C struct representing a single row of data, containing fields like an integer ID and a dynamically allocated string name.

**What is a table?**
A C struct that manages a dynamic array of records, tracking its current capacity and record count.

**Why is persistence needed?**
Because RAM is volatile and clears when the program closes. Persistence saves data to a hard drive file so it survives a restart.

**How is a record represented in memory?**
As a `struct Record` allocated on the heap, with a pointer to a separate heap allocation for the name string.

**How is a record represented on disk?**
As a sequence of raw bytes written using `fwrite`, structured in a specific order so we can read it back.

**Why is name dynamically allocated?**
To avoid wasting memory with large fixed-size arrays and to prevent artificial limits on string length.

**What owns allocated memory?**
The entity that created it. The `Record` owns the name string, and the `Table` owns the array of records.

**Why do count and capacity both exist?**
`count` tracks actual data, while `capacity` tracks allocated memory slots so we know when we need to use `realloc` to grow the array.

**How does dynamic array growth work?**
When count reaches capacity, we ask the OS for a new, larger block of memory (usually double the size), copy the old data over, and free the old block.

**What happens when malloc fails?**
It returns a `NULL` pointer. We must check for this and safely abort or return an error instead of crashing.

**What is the difference between . and ->?**
`.` accesses members of a direct struct object, while `->` accesses members of a struct through a pointer.

**What does the parser do?**
It converts raw user text into structured command objects that the program can easily read.

**What does the executor do?**
It takes the parsed commands and triggers the actual C functions that manipulate tables and memory.

**Why separate parser and storage?**
So that if we change how data is stored on disk, we don't have to rewrite the code that understands user text. Separation of concerns.

**How does SELECT work?**
The executor loops through the table's records in memory and prints their data to the screen.

**How does WHERE work?**
It acts as a filter during a loop; the executor checks a condition (like `id == 5`) before deciding to operate on a record.

**Why is linear search O(n)?**
Because in the worst case, it has to check every single record in the array one by one to find a match.

**How does the index work? (Future phase)**
It keeps a separate, sorted list of IDs and their locations, allowing us to find them quickly using binary search.

**Why can binary search be O(log n)?**
Because it repeatedly halves the search space in a sorted array, vastly reducing the number of checks needed.

**What is the cost of maintaining an index?**
Every time we insert, update, or delete a record, we also have to spend time sorting and updating the index.

**How does persistence survive restart?**
When the program starts, it uses `fopen` and `fread` to load the bytes from the `.db` file back into memory before waiting for user input.

**What happens on corrupted data?**
The program should detect unexpected layouts (like a negative capacity) and safely report an error rather than executing undefined behavior.

**Why not full SQL?**
Full SQL (with joins, subqueries, etc.) requires massive parsing engines and optimizers, which is out of scope for a learning MVP.

**Why not B-Tree initially?**
B-Trees are highly complex data structures. It's better to build a working engine with linear search first, then optimize it.

**What are the limitations?**
No concurrent access, no networking, no transactions, and only simple filtering.

**What would you implement next?**
A B-Tree index for faster lookups, or a Write-Ahead Log (WAL) to prevent data corruption if the power goes out.
