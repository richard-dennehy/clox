#include <string.h>
#include "table.h"
#include "object.h"

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(VM* vm, Table* table) {
    VM_FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, uint32_t capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;
    // open addressing - entry may not be in the expected location
    for (;;) {
        Entry* entry = &entries[index];
        if (!entry->key) {
            // not present; return first tombstone found, or first empty slot
            if (IS_NIL(entry->value)) {
                return tombstone ? tombstone : entry;
            } else {
                if (!tombstone) tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjustCapacity(VM* vm, Table* table, uint32_t newCapacity) {
    Entry* entries = VM_ALLOCATE(Entry, newCapacity);

    for (uint32_t i = 0; i < newCapacity; ++i) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    // need to redistribute entries as the indexes depend on the capacity
    for (uint32_t i = 0; i < table->capacity; ++i) {
        Entry* entry = &table->entries[i];
        if (!entry->key) continue;

        Entry* dest = findEntry(entries, newCapacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    VM_FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = newCapacity;
}

bool tableSet(VM* vm, Table* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        uint32_t capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(vm, table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableGet(Table* table, ObjString* key, Value* value) {
    if (!table->count) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (!entry->key) return false;

    *value = entry->value;
    return true;
}

void tableAddAll(VM* vm, Table* from, Table* to) {
    for (uint32_t i = 0; i < from->capacity; ++i) {
        Entry* entry = &from->entries[i];
        if (entry->key) {
            tableSet(vm, to, entry->key, entry->value);
        }
    }
}

bool tableDelete(Table* table, ObjString* key) {
    if (!table->count) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (!entry->key) return false;

    entry->key = NULL;
    // flag as tombstone
    entry->value = BOOL_VAL(true);
    return true;
}

ObjString* tableFindString(Table* table, const char* chars, uint32_t length, uint32_t hash) {
    if (!table->count) return false;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry* entry = table->entries + index;
        if (!entry->key) {
            // exclude tombstones
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length && entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}

void markTable(VM* vm, Table* table) {
    for (uint32_t i = 0; i < table->capacity; i++) {
        Entry* entry = table->entries + i;
        markObject(vm, (Obj*) entry->key);
        markValue(vm, entry->value);
    }
}

void tableRemoveWhite(Table* table) {
    for (uint32_t i = 0; i < table->capacity; i++) {
        Entry* entry = table->entries + i;
        if (entry->key && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}
