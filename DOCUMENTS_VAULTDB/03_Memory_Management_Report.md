# Document 3: Memory Management Report

> **Last updated:** After bug-fix pass on 2026-10-05. All three real bugs identified in the initial audit have been corrected in the source code. The project now compiles with zero warnings.

## Table of Contents
1. [Complete Allocation Inventory](#1-complete-allocation-inventory)
2. [Ownership Chain](#2-ownership-chain)
3. [Bug Audit: Initial Findings and Fix Status](#3-bug-audit-initial-findings-and-fix-status)

---

## 1. Complete Allocation Inventory

Every `malloc` and `realloc` in the project, in the order they appear during a typical session.

| # | Location | What is allocated | Size formula | Freed in | Notes |
|---|---|---|---|---|---|
| 1 | `table_create` | Table struct | `sizeof(Table)` | `table_destroy` → `free(table)` | Outermost allocation |
| 2 | `table_create` | Table name string | `strlen(name) + 1` | `table_destroy` → `free(table->name)` | Owned by Table |
| 3 | `table_create` | Records array block | `cap * sizeof(Record)` where cap = max(initial_capacity, 1) | `table_destroy` → `free(table->records)` | Holds all records by value |
| 4 | `table_insert` (growth) | Grown records array | `newCapacity * sizeof(Record)` | `table_destroy` → `free(table->records)` | Replaces allocation #3 via realloc |
| 5 | `table_insert` | Each record's name string | `strlen(name) + 1` | `table_destroy` loop → `free(records[i].name)` | One allocation per inserted record |
| 6 | `storage_load_table` → `read_string` | Temporary table name buffer | length read from file | Freed immediately after `table_create` | Short-lived; table_create copies it |
| 7 | `storage_load_table` → `read_string` | Temporary record name buffer | length read from file | Freed immediately after `table_insert` | Short-lived; table_insert copies it |
| 8 | `record_create` | Record struct | `sizeof(Record)` | `record_destroy` | NOT called in current code flow |
| 9 | `record_create` | Record name string | `strlen(name) + 1` | `record_destroy` | NOT called in current code flow |

> **Note on #8 and #9:** `record_create` and `record_destroy` are implemented but not called anywhere in the current program. Records are managed directly through the Table's array. These functions exist as a clean standalone API.

---

## 2. Ownership Chain

```
Table (heap)
├── table->name (heap)           owned by Table, freed in table_destroy
├── table->records (heap)        owned by Table, freed in table_destroy
│   ├── records[0].name (heap)   owned by records array, freed in table_destroy loop
│   ├── records[1].name (heap)   owned by records array, freed in table_destroy loop
│   └── ...
```

The correct free order inside `table_destroy` is always innermost to outermost:
1. `records[i].name` strings (leaves)
2. `table->records` array (branch)
3. `table->name` string (branch)
4. `table` struct (root)

Freeing in any other order risks reading freed memory or double-freeing.

---

## 3. Bug Audit: Initial Findings and Fix Status

### ISSUE 1 — Potential leak in `table_insert` on name malloc failure

**Status: ✅ Not a real bug — False alarm.**

If `malloc` for `slot->name` fails, `record_count` has not yet been incremented, so `table_destroy` will not try to free that slot's name. The slot itself lives inside the array block which is freed as a unit. No leak occurs.

---

### ISSUE 2 — `table_create` with `initial_capacity = 0`

**Status: ✅ FIXED in `src/table.c`**

**Was:** `malloc(initial_capacity * sizeof(Record))` called `malloc(0)` when `initial_capacity` was 0 (e.g., when loading an empty saved table). `malloc(0)` may return NULL on some platforms, causing a false failure.

**Fix applied:**
```c
int cap = (initial_capacity > 0) ? initial_capacity : 1;
table->records = malloc(cap * sizeof(Record));
...
table->capacity = cap;
```
Now a minimum of 1 slot is always allocated. Saving and loading an empty table works correctly.

---

### ISSUE 3 — `.load` destroys the current table before confirming load succeeds

**Status: ✅ FIXED in `src/main.c`**

**Was:**
```c
if (active_table != NULL) table_destroy(active_table); // old table GONE
active_table = storage_load_table("data/vault.db");    // may return NULL
```
If the load failed, the old table was already destroyed with no recovery possible.

**Fix applied:**
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
The old table is now only destroyed after a successful load is confirmed. On failure, the message clearly states the existing table is unchanged.

---

### ISSUE 4 — `fwrite` return values not checked in `storage_save_table`

**Status: ✅ FIXED in `src/storage.c`**

**Was:** All `fwrite` calls ignored their return value. A disk-full error would silently produce a corrupt `.db` file and still return 0 (success).

**Fix applied:** Every `fwrite` call is now checked:
```c
if (fwrite(&name_len, sizeof(int), 1, file) != 1) { fclose(file); return -1; }
if (fwrite(table->name, sizeof(char), name_len, file) != (size_t)name_len) { fclose(file); return -1; }
// ... and so on for every field
```
On any write failure, the file is closed and the function returns -1. The caller in `main.c` already prints an appropriate error on -1.

---

### ISSUE 5 — `record_create` and `record_destroy` are dead code

**Status: ℹ️ Acknowledged, no fix needed.**

Both functions are correct and compile cleanly. They exist as a public API for standalone Record management if the project evolves. No action required.

---

### Final Summary

| Issue | Severity | Type | Status |
|---|---|---|---|
| table_insert name-malloc failure | None | False alarm | ✅ N/A |
| table_create with capacity=0 | Medium | Edge-case bug | ✅ Fixed |
| .load destroys table before confirming success | Medium | Data-loss risk | ✅ Fixed |
| fwrite return values not checked | Low | Correctness | ✅ Fixed |
| record_create / record_destroy unused | None | Dead code | ✅ Acknowledged |

**All real bugs have been corrected. The project compiles with zero warnings after the fix pass.**
