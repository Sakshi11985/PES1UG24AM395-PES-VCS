#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Load index from .pes/index
int index_load(Index *index) {

    index->count = 0;

    FILE *f = fopen(".pes/index", "r");

    // If no index file → empty index
    if (!f) return 0;

    char line[1024];

    while (fgets(line, sizeof(line), f)) {

        if (index->count >= MAX_INDEX_ENTRIES)
            break;

        IndexEntry *e = &index->entries[index->count];

        char hash_hex[HASH_HEX_SIZE + 1];

        int parsed = sscanf(line, "%o %64s %lu %u %[^\n]",
                            &e->mode,
                            hash_hex,
                            &e->mtime_sec,
                            &e->size,
                            e->path);

        if (parsed != 5)
            continue;

        // convert hex → ObjectID
        if (hex_to_hash(hash_hex, &e->hash) != 0)
            continue;

        index->count++;
    }

    fclose(f);
    return 0;
}


// Save index to .pes/index
int index_save(const Index *index) {

    FILE *f = fopen(".pes/index", "w");
    if (!f) return -1;

    for (int i = 0; i < index->count; i++) {

        const IndexEntry *e = &index->entries[i];

        char hash_hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&e->hash, hash_hex);

        fprintf(f, "%o %s %lu %u %s\n",
                e->mode,
                hash_hex,
                e->mtime_sec,
                e->size,
                e->path);
    }

    fclose(f);
    return 0;
}


// Add file to index
int index_add(Index *index, const char *path) {

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, data, size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    // find existing entry
    IndexEntry *e = index_find(index, path);

    if (!e) {
        e = &index->entries[index->count++];
    }

    e->mode = 0100644;
    e->hash = id;
    e->size = size;
    strcpy(e->path, path);
    e->mtime_sec = 0;

    return 0;
}


// Remove entry
int index_remove(Index *index, const char *path) {

    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {

            for (int j = i; j < index->count - 1; j++) {
                index->entries[j] = index->entries[j + 1];
            }

            index->count--;
            return 0;
        }
    }

    return -1;
}


// Find entry
IndexEntry* index_find(Index *index, const char *path) {

    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }

    return NULL;
}


// Simple status
int index_status(const Index *index) {

    printf("Staged changes:\n");

    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
    }

    printf("\nUnstaged changes:\n");
    printf("(nothing to show)\n");

    printf("\nUntracked files:\n");
    printf("(nothing to show)\n");

    return 0;
}
