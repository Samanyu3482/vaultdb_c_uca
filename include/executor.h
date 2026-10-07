#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"
#include "table.h"

void execute_command(Command cmd, Table **active_table);

#endif 

