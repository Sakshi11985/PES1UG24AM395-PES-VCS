int index_load(Index *index) {

    index->count = 0;

    FILE *f = fopen(".pes/index", "r");

    // If file doesn't exist → empty index
    if (!f) return 0;

    char line[1024];

    while (fgets(line, sizeof(line), f)) {

        if (index->count >= MAX_INDEX_ENTRIES)
            break;

        IndexEntry *e = &index->entries[index->count];

        char hash_hex[HASH_HEX_SIZE + 1];

        // Try parsing safely
        int parsed = sscanf(line, "%o %64s %ld %zu %[^\n]",
                            &e->mode,
                            hash_hex,
                            &e->mtime_sec,
                            &e->size,
                            e->path);

        if (parsed != 5)
            continue;   // skip bad lines

        // Convert hash
        if (hex_to_hash(hash_hex, &e->id) != 0)
            continue;

        index->count++;
    }

    fclose(f);

    // ALWAYS SUCCESS
    return 0;
}
