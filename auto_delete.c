#include "auto_delete.h"

int isDirectory(const char* path) {
    struct stat path_stat;
    if(stat(path, &path_stat) != 0) {
        return 0;
    }
    return S_ISDIR(path_stat.st_mode);
}
void check_file_and_delete_time(const char* path, int duration, char flag) {
    struct dirent *de;
    struct stat path_stat;
    stat(path, &path_stat);
    DIR *dr = opendir(path);
    time_t current_time = time(NULL);
    time_t final_time;
    switch(flag) {
        case 'd':
            final_time = current_time - (duration * 24 * 60 * 60);
            break;
        case 'h':
            final_time = current_time - (duration * 60 * 60);
            break;
        case 'm':
            final_time = current_time - (duration * 60);
            break;
        default:
            final_time = current_time - duration;
            break;
    }
    if(dr == NULL) {
        fprintf(stderr, "Couldn't open the directory: %s\n", path);
        return;
    }
    while((de = readdir(path)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        char* file_path = malloc(strlen(path) + strlen(de->d_name) + 2);
        snprintf(file_path, strlen(path) + strlen(de->d_name) + 2, "%s/%s", path, de->d_name);
        if(isDirectory(file_path)) {
            check_file_and_delete_time(file_path, duration, flag);
        } else {
            if(path_stat.st_ctime > final_time) {
                remove(file_path);
                printf("Deleted: %s | Creation Date: %lld\n", file_path, ctime(path_stat.st_ctime));
            }
        }
    }
}
void check_file_and_delete_size(const char* path, int size, char flag) {
    struct dirent *de;
    struct stat statbuf;
    DIR *dr = opendir(path);
    long long int size_in_bytes;

    switch(flag) {
        case 'k':  
            size_in_bytes = size * 1024;
            break;
        case 'm':
            size_in_bytes = size * 1024 * 1024;
            break;
        case 'g':
            size_in_bytes = size * 1024 * 1024 * 1024;
            break;
        default:
            size_in_bytes = size;
            break; 
    }

    if(dr == NULL) {
        fprintf(stderr, "Couldn't open the directory: %s\n", path);
        return;
    }

    while((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        char* file_path = malloc(strlen(path) + strlen(de->d_name) + 2);
        snprintf(file_path, strlen(path) + strlen(de->d_name) + 2, "%s/%s", path, de->d_name);

        if(isDirectory(file_path)) {

            check_file_and_delete_size(file_path, size, flag);
        } else {

            if(stat(file_path, &statbuf) == 0) {
                if(statbuf.st_size > size_in_bytes) {
                    remove(file_path);
                    printf("Deleted %s | Size: %lld Bytes \n", file_path, statbuf.st_size);
                }
            }
        }
        free(file_path);
    }

    closedir(dr);
}


