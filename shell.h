#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define ALPHABET_SIZE 26
#define MAX_INPUT 1024
#define MAX_ARGUMENTS 100
#define MAX_WORDS_LENGTH 100
#define COMMAND_SIZE 128
#define MAX_JOBS 64

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

typedef enum {
    RUNNING,
    STOPPED,
    TERMINATED
} job_status;

typedef struct {
    pid_t pid;
    int job_id;
    job_status status;
    int priority;
    char command[COMMAND_SIZE];
} Job;


typedef struct Node {
    char command[COMMAND_SIZE];
    struct Node *next;
    struct Node *prev;
} Node;

typedef struct TrieNode {
	struct TrieNode* children[ALPHABET_SIZE];
	bool is_end;
} TrieNode;

TrieNode* createNode();
void insert_into_trie(const char* command);
void collect_words(TrieNode* node, char* prefix, int length);
void search_prefix(char* prefix);
void enableRawMode();
void disableRawMode();
Node* add_to_history(Node *head, const char *command);
void prompt();
void sysusage();
void change_directory(char **args);
void handle_redirection(char **args, char *file);
void handle_pipeline(char **command1, char **command2);
void exec_command(char *input);
void readInput(char *buffer);
void free_history(Node *head);

#endif