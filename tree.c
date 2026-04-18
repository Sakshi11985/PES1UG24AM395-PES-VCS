#include "tree.h"
#include "index.h"
#include "object.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Compare function for sorting entries by name (required before serializing)
static int entry_cmp(const void *a, const void *b) {
    const TreeEntry *ea = (const TreeEntry *)a;
    const TreeEntry *eb = (const TreeEntry *)b;
    return strcmp(ea->name, eb->name);
}

// Serialize a Tree into raw bytes:
// Each entry is packed as:  [mode: 4 bytes][name: null-terminated][hash: sizeof(ObjectID)]
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    if (!tree || !data_out || !len_out) return -1;

    // Make a sorted copy (entries MUST be sorted by name per spec)
    Tree sorted = *tree;
    qsort(sorted.entries, sorted.count, sizeof(TreeEntry), entry_cmp);

    // Calculate total buffer size
    size_t total = 0;
    for (int i = 0; i < sorted.count; i++) {
        total += sizeof(uint32_t);              // mode
        total += strlen(sorted.entries[i].name) + 1;  // name + '\0'
        total += sizeof(ObjectID);              // hash
    }

    uint8_t *buf = malloc(total);
    if (!buf) return -1;

    uint8_t *ptr = buf;
    for (int i = 0; i < sorted.count; i++) {
        TreeEntry *e = &sorted.entries[i];

        // Write mode (4 bytes, big-endian style raw copy)
        memcpy(ptr, &e->mode, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        // Write name (null-terminated)
        size_t name_len = strlen(e->name) + 1;
        memcpy(ptr, e->name, name_len);
        ptr += name_len;

        // Write hash
        memcpy(ptr, &e->hash, sizeof(ObjectID));
        ptr += sizeof(ObjectID);
    }

    *data_out = buf;
    *len_out  = total;
    return 0;
}

// Parse raw bytes back into a Tree struct (reverse of tree_serialize)
int tree_parse(const void *data, size_t len, Tree *tree_out) {
    if (!data || !tree_out) return -1;

    tree_out->count = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = ptr + len;

    while (ptr < end) {
        if (tree_out->count >= MAX_TREE_ENTRIES) return -1;
        TreeEntry *e = &tree_out->entries[tree_out->count];

        // Read mode
        if (ptr + sizeof(uint32_t) > end) return -1;
        memcpy(&e->mode, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        // Read null-terminated name
        const uint8_t *name_start = ptr;
        while (ptr < end && *ptr != '\0') ptr++;
        if (ptr >= end) return -1;  // no null terminator found
        size_t name_len = ptr - name_start;
        if (name_len >= sizeof(e->name)) return -1;
        memcpy(e->name, name_start, name_len);
        e->name[name_len] = '\0';
        ptr++;  // skip '\0'

        // Read hash
        if (ptr + sizeof(ObjectID) > end) return -1;
        memcpy(&e->hash, ptr, sizeof(ObjectID));
        ptr += sizeof(ObjectID);

        tree_out->count++;
    }

    return 0;
}

// Build a Tree from the current index and write it to the object store
int tree_from_index(ObjectID *id_out) {
    Index index;
    index_load(&index);

    Tree tree;
    tree.count = 0;

    for (int i = 0; i < index.count; i++) {
        TreeEntry *entry = &tree.entries[tree.count];
        IndexEntry *ie   = &index.entries[i];
        entry->mode = ie->mode;
        strcpy(entry->name, ie->path);
        entry->hash = ie->hash;
        tree.count++;
    }

    void *data;
    size_t len;
    if (tree_serialize(&tree, &data, &len) != 0)
        return -1;

    if (object_write(OBJ_TREE, data, len, id_out) != 0) {
        free(data);
        return -1;
    }

    free(data);
    return 0;
}
