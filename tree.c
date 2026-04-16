#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Minimal working version just to pass build + tests

int tree_from_index(ObjectID *id_out) {
    Index idx;

    if (index_load(&idx) != 0) return -1;

    Tree tree;
    tree.count = 0;

    for (int i = 0; i < idx.count; i++) {
        TreeEntry *t = &tree.entries[tree.count];
        IndexEntry *e = &idx.entries[i];

        t->mode = e->mode;
        strcpy(t->name, e->path);
        t->hash = e->id;

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
