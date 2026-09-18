#ifndef RECORD_H
#define RECORD_H

struct Record {
    int id;
    char *name;
    int price;
    int stock;
};

struct Record *record_create(int id, const char *name, int price, int stock);

void record_destroy(struct Record *record);

#endif
