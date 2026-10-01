#include <stdio.h>
#include"record.h"

int main(void) {
    printf("VaultDB starting...\n");
    Record *record = record_create(1,"item1",10,100);
    if(record == NULL) {
        printf("Record not created\n");
        return 1;
    }
    printf("Record created successfully\n");
    record_destroy(record);
    printf("Record destroyed successfully\n");
    return 0;
}
