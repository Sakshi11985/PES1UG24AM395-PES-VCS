#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tree_from_index(ObjectID *id_out) {

    Index index;
    index_load(&index);

    Tree tree;
    tree.count = 0;

    for (int i = 0; i < index.count; i++) {

        TreeEntry *entry = &tree.entries[tree.count];
        IndexEntry *ie = &index.entries[i];

        entry->mode = ie->mode;

        // copy file name
        strcpy(entry->name, ie->path);

        // ✅ CORRECT LINE (THIS FIXES YOUR ERROR)
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
