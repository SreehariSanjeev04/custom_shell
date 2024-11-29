#ifndef AUTO_DELETE_H
#define AUTO_DELETE_H

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int isDirectory(const char* path);
void check_file_and_delete_size(const char* path, int size, char flag);
void check_file_and_delete_time(const char* path, int time, char flag);

#endif