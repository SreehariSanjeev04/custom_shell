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

#define ALPHABET_SIZE 26
#define MAX_INPUT 1024
#define MAX_ARGUMENTS 100
#define MAX_WORDS_LENGTH 100
#define COMMAND_SIZE 128

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

// Node structure for command history
typedef struct Node {
    char command[COMMAND_SIZE];
    struct Node *next;
    struct Node *prev;
} Node;

// Implementing Trie for autocomplete feature

typedef struct TrieNode {
	struct TrieNode* children[ALPHABET_SIZE];
	bool is_end;
} TrieNode;

//globals
Node *current = NULL;
Node *history_head = NULL;
TrieNode* autocomplete_head = NULL;
struct termios orig_termios;
struct sysinfo memInfo;
struct utsname unameData;
char* common_commands[] = {
	"cd", "pwd", "ls", "history", "exit", "clear", "echo", "help", "uname", "top", "whoami", "whatisthis", NULL
};

TrieNode* createNode() {
	TrieNode* node = (TrieNode*)malloc(sizeof(TrieNode));
    if (!node) {
        perror("Memory error");
        exit(EXIT_FAILURE);
    }
    node->is_end = false;
	for (int i = 0; i < ALPHABET_SIZE; i++) {
        node->children[i] = NULL;
    }
	return node;
}

void insert_into_trie(const char* command) {
	if(!autocomplete_head) {
		autocomplete_head = createNode();
	}
	TrieNode* current = autocomplete_head;
	while(*command) {
		int index = tolower(*command) - 'a';
		if (!current->children[index]) {
            current->children[index] = createNode();
        }
		current = current->children[index];
		command++;
	}
	current->is_end = true;
}

void collect__words(TrieNode* node, char* prefix, char** result, int* index) {
	if(node->is_end) {
		strcpy(result[*index], prefix);
        (*index)++;
	}
	else {
		for(int i = 0; i < ALPHABET_SIZE; i++) {
            if(node->children[i]) {
				int length = strlen(prefix);
                prefix[length] = 'a' + i;
				prefix[length+1] = '\0';
                collect__words(node->children[i], prefix, result, index);
                prefix[length] = '\0';
            }
        }
	}
}

bool search_prefix(char* prefix, char** result, int *result_index) {
	TrieNode* current = autocomplete_head;
    while(*prefix) {
        int index = tolower(*prefix) - 'a';
        if (!current->children[index]) {
			return false;
        }
        current = current->children[index];
        prefix++;
    }
	for(int i = 0; i < ALPHABET_SIZE; i++) {
		if(current->children[i]) {
			collect__words(current->children[i], prefix, result, result_index);
		}
	}
	return true;
}


// Enable raw mode for terminal input
void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

// Disable raw mode and restore original terminal settings
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// Add a command to the history list
Node* add_to_history(Node *head, const char *command) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->command, command);
    new_node->next = head;
    new_node->prev = NULL;
    if (head) {
        head->prev = new_node;
    }
    return new_node;
}

// Display the shell prompt
void prompt() {
    char *user = getenv("USER");
    if (user != NULL) {
        printf("%s@", user);
    } else {
        printf("anonymous@");
    }
    if (uname(&unameData) == 0) {
        printf("%s$ ", unameData.sysname);
    } else {
        printf("unknownOS$ ");
    }
}

// Display system memory usage
void sysusage() {
    sysinfo(&memInfo);
    long long totalPhysMem = memInfo.totalram;
    long long usedPhysMem = memInfo.totalram - memInfo.freeram;
    usedPhysMem *= memInfo.mem_unit;
    totalPhysMem *= memInfo.mem_unit;
    printf("Total Physical Memory: %.2f MB\n", totalPhysMem / (1024.0 * 1024.0));
    printf("Used Physical Memory: %.2f MB\n", usedPhysMem / (1024.0 * 1024.0));
}

// Execute shell commands
void exec_command(char *input) {
    char *args[MAX_ARGUMENTS];
    int arg_count = 0;
    char *token = strtok(input, " ");
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) return;

    if (strcmp(args[0], "whatisthis") == 0) {
        printf("This is a custom shell developed in C due to the fact that the developer was bored.\n");
        return;
    }
    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell.\n");
        exit(EXIT_SUCCESS);
    }
    if (strcmp(args[0], "sysusage") == 0) {
        sysusage();
        return;
    }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            printf("No directory provided.\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("Failed to change directory");
            }
        }
        return;
    }

    // Fork a child process to execute the command
    pid_t pid = fork();
    if (pid < 0) {
        perror("Failed to fork");
        return;
    }
    if (pid == 0) {
        execvp(args[0], args);
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

// Read user input with raw mode
void readInput(char *buffer) {
    int index = 0;
    char c;

    while (1) {
        c = getchar();
        if (c == '\n') {
            buffer[index] = '\0';
            printf("\n");
            break;
        } else if (c == 127) {  // Backspace
            if (index > 0) {
                printf("\b \b");
                index--;
            }
        } else if(c == '\t') {
			int result_index = 0;
			char* autocomplete_result[ALPHABET_SIZE];
			if(search_prefix(buffer, autocomplete_result, &result_index)) {
				if (result_index > 0) {
					strcpy(buffer, autocomplete_result[0]); 
					index = strlen(buffer);  
					printf("\r\033[K");
					prompt();
					printf("%s", buffer);
				}
			} 
			buffer[index] = '\0';
			printf("\r\033[K");
			prompt();
			printf("%s", buffer);
		} 
		else if (c == '\033') {  
            getchar();  
            switch (getchar()) {
                case 'A':  // Up arrow
                    if (current && current->next) {
                        current = current->next;
                        index = strlen(current->command);
                        strcpy(buffer, current->command);
						printf("\r\033[K");
						prompt();
						printf("%s", buffer);
                    }
                    break;
                case 'B':  // Down arrow
                    if (current && current->prev) {
                        current = current->prev;
                        index = strlen(current->command);
                        strcpy(buffer, current->command);
                        printf("\r\033[K");
						prompt();
						printf("%s", buffer);
                    }
                break;
            }
        } else {
            if (index < MAX_INPUT - 1) {
                buffer[index++] = c;
                putchar(c);
            }
        }
    }
}



// Free history nodes
void free_history(Node *head) {
    while (head) {
        Node *temp = head;
        head = head->next;
        free(temp);
    }
}

int main() {
    char input[MAX_INPUT];
    history_head = NULL;

    atexit(disableRawMode); 
    enableRawMode();


	    // adding common commands to the trie
    int trie_index = 0;
    while (common_commands[trie_index] != NULL) {
        insert_into_trie(common_commands[trie_index++]);
    }

    while (1) {
        prompt();
        readInput(input);
        add_to_history(history_head, input);
        exec_command(input);
    }

    free_history(history_head);
    return 0;
}
