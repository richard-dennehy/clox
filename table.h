#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "value.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    uint32_t count;
    uint32_t capacity;
    Entry* entries;
    FreeList* freeList;
} Table;

void initTable(FreeList* freeList, Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, uint32_t length, uint32_t hash);

#endif //CLOX_TABLE_H
