#include "focus_mode.h"

const char *BLOCKED_HOSTS[] = {
    "x.com"
};

void check_root() {
    uid_t euid = geteuid(); 
    if (euid != 0) {
        fprintf(stderr, "This script must be run as root\n");
        exit(EXIT_FAILURE);
    }
}

void copy_contents(const char* src, const char* dest) {
    FILE *src_file = fopen(src, "r");
    FILE *dest_file = fopen(dest, "w");
    if (!src_file || !dest_file) {
        fprintf(stderr, "Couldn't open HOST OR BACKUP HOST file.\n");
        exit(EXIT_FAILURE);
    }
    char c;
    while ((c = fgetc(src_file)) != EOF) {
        fputc(c, dest_file);
    }
    fclose(dest_file);
    fclose(src_file);
    printf("Successfully backed up initial state.\n");
}

void enable_focus_mode() {
    check_root();
    FILE* hosts_file = fopen(HOSTS_FILE, "a");
    if (!hosts_file) {
        fprintf(stderr, "Failed to open hosts file\n");
        exit(EXIT_FAILURE);
    }

    copy_contents(HOSTS_FILE, BACKUP_FILE);

    printf("Adding blocked sites to /etc/hosts...\n");
    for (int i = 0; i < BLOCKED_SITES; i++) {
        fprintf(hosts_file, "127.0.0.1 %s\n", BLOCKED_HOSTS[i]);
    }

    fclose(hosts_file);
    printf("Focus mode enabled.\n");
}

void disable_focus_mode() {
    copy_contents(BACKUP_FILE, HOSTS_FILE);
    printf("Focus mode disabled.\n");
}
