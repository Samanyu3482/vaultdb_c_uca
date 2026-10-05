#ifndef PARSER_H
#define PARSER_H

typedef enum {
    CMD_UNKNOWN,
    CMD_CREATE_TABLE,
    CMD_INSERT,
    CMD_SELECT,
    CMD_UPDATE,
    CMD_DELETE
} CommandType;

typedef struct {
    CommandType type;
    char table_name[256];

   
    int id;
    char record_name[256];
    int price;
    int stock;

  
    int has_where;           
    char where_column[256];   
    int where_value;   
    
    char set_column[256];
    int set_value_int;
} Command;

Command parse_command(const char *query);

#endif 


