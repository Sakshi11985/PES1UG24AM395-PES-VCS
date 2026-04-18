// pes.c — Main entry point for PES-VCS

#include "pes.h"
#include "index.h"
#include "tree.h"
#include "commit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Initialize repository
int cmd_init() {
    mkdir(".pes", 0755);
    mkdir(".pes/objects", 0755);
    mkdir(".pes/refs", 0755);
    mkdir(".pes/refs/heads", 0755);
    FILE *f = fopen(".pes/HEAD", "w");
    if (f) {
        fprintf(f, "ref: refs/heads/main\n");
        fclose(f);
    }
    f = fopen(".pes/refs/heads/main", "w");
    if (f) fclose(f);
    printf("Initialized empty PES repository in .pes/\n");
    return 0;
}

// Add files
int cmd_add(int argc, char *argv[]) {
    Index index;
    index_load(&index);
    for (int i = 2; i < argc; i++) {
        if (index_add(&index, argv[i]) != 0) {
            fprintf(stderr, "error: failed to add %s\n", argv[i]);
            return -1;
        }
    }
    if (index_save(&index) != 0) {
        fprintf(stderr, "error: failed to save index\n");
        return -1;
    }
    return 0;
}

// Status
int cmd_status() {
    Index index;
    index_load(&index);
    return index_status(&index);
}

// Commit
int cmd_commit(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[2], "-m") != 0) {
        fprintf(stderr, "usage: pes commit -m \"message\"\n");
        return -1;
    }
    const char *message = argv[3];
    ObjectID commit_id;
    if (commit_create(message, &commit_id) != 0) {
        fprintf(stderr, "error: commit failed\n");
        return -1;
    }
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&commit_id, hex);
    printf("[main %.8s] %s\n", hex, message);
    return 0;
}

// Log callback — prints one commit to stdout
static void log_print(const ObjectID *id, const Commit *c, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    printf("commit %s\n", hex);
    printf("Author: %s\n", c->author);
    printf("Date:   %" PRIu64 "\n\n", c->timestamp);
    printf("    %s\n\n", c->message);
}

// Log
int cmd_log() {
    return commit_walk(log_print, NULL);
}

// MAIN
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: pes <command>\n");
        return -1;
    }
    if (strcmp(argv[1], "init") == 0) {
        return cmd_init();
    }
    else if (strcmp(argv[1], "add") == 0) {
        return cmd_add(argc, argv);
    }
    else if (strcmp(argv[1], "status") == 0) {
        return cmd_status();
    }
    else if (strcmp(argv[1], "commit") == 0) {
        return cmd_commit(argc, argv);
    }
    else if (strcmp(argv[1], "log") == 0) {
        return cmd_log();
    }
    else {
        fprintf(stderr, "unknown command: %s\n", argv[1]);
        return -1;
    }
}
