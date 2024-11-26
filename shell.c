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
Node *history_head;
TrieNode* autocomplete_head = NULL;
struct termios orig_termios;
struct sysinfo memInfo;
struct utsname unameData;
char* common_commands[] = {
	"cd", "pwd", "ls", "exit", "clear", "echo", "help", "uname", "top", "whoami", "whatisthis", 
    "kill", "service", "gcc",
    NULL
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
	TrieNode* temp = autocomplete_head;
	while(*command) {
		int index = tolower(*command) - 'a';
		if (!temp->children[index]) {
            temp->children[index] = createNode();
        }
		temp = temp->children[index];
		command++;
	}
	temp->is_end = true;
}


void collect_words(TrieNode* node, char* prefix, int length) {
    if(node->is_end) {
        prefix[length] = '\0';
        printf("%s\n", prefix);
    }
    for(int i = 0; i < ALPHABET_SIZE; i++) {
        if(node->children[i]) {
            prefix[length] = 'a' + i;
            collect_words(node->children[i], prefix, length+1);
        }
    }
}

void search_prefix(char* prefix) {
    char* temp_string = strdup(prefix);
    if(!autocomplete_head) return;
    TrieNode* temp = autocomplete_head;
    int prefix_length = 0;
    while(*prefix) {
        int index = tolower(*prefix) - 'a';
        if(index < 0 || index >= ALPHABET_SIZE || !temp->children[index]) {
            return;
        }
        temp = temp->children[index];
        prefix++;
        prefix_length++;
    }
    collect_words(temp, temp_string, prefix_length);
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

void change_directory(char **args) {
    if(args[1] == NULL) {
        printf("No directory provided.\n");
    }
    if (chdir(args[1])!= 0) {
        perror("Failed to change directory");
    }
    return;
}

void handle_redirection(char **args, char *file) {
    int fd = 0;
    if(file) {
        fd = open(file, O_WRONLY | O_CREAT);
        if(fd < 0) {
            perror("Failed to open file for redirection");
            return;
        } else {
            int saved_stdout = dup(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            // run the process
            pid_t pid = fork();
            if(pid < 0) {
                perror("Failed to fork.");
                exit(EXIT_FAILURE);
            } else if(pid == 0) {
                execvp(args[0], args);
                perror("Failed to execute command");
                exit(EXIT_FAILURE);
            } else {
                wait(NULL);
            }

            // restore stdout  
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
    } else {
        printf("No file provided for redirection.\n");
        return;
    }
}

void handle_pipeline(char **command1, char **command2) {
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }
    // pipefd[0] is the read end and the pipefd[1] is the write end

    pid1 = fork();
    if (pid1 == 0) {
        // In the first child process, execute the first command
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the pipe
        close(pipefd[0]); 
        execvp(command1[0], command1); 
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        // In the second child process, execute the second command
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the pipe
        close(pipefd[1]);  
        execvp(command2[0], command2); 
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    wait(NULL);
    wait(NULL);
}


// Execute shell commands
void exec_command(char *input) {
    bool redirection = false;
    bool pipeline = false;
    char* file = NULL;
    char *args[MAX_ARGUMENTS];
    char *command1[MAX_ARGUMENTS];
    char *command2[MAX_ARGUMENTS];
    int arg_count = 0;
    int command_two_idx = 0;
    char *token = strtok(input, " ");
    
    while (token != NULL) {
        if (*token == '|') {
            // Pipeline detected, split the commands
            pipeline = true;
            command1[arg_count] = NULL;
            // Store the left part of the pipeline
            token = strtok(NULL, " ");
            break;
        }

        if (*token == '>') {
            // Handle redirection
            redirection = true;
            file = strtok(NULL, " ");
            break;
        }
        command1[arg_count] = token;
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;
    if (pipeline) {
        int i = 0;
        while (token != NULL) {
            command2[i++] = token;
            token = strtok(NULL, " ");
        }
        command2[i] = NULL;
        handle_pipeline(command1, command2);
        return; 
    }
    // check for redirection
    if(redirection) {
        printf("Redirection detected.\n");
        handle_redirection(args, file);
        redirection = false;
        return;
    }
    if (strcmp(args[0], "whatisthis") == 0) {
        printf("This is a custom shell developed in C due to the fact that the developer was bored.\n");
        return;
    }
    if (strcmp(args[0], "help") == 0) {
        printf("Refer to https://github.com/SreehariSanjeev04/custom_shell for the commands. Use 'man' for more information regarding a command. \n");
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
        change_directory(args);
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
    current = history_head;
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
            buffer[index] = '\0';
            printf("\nSuggestions: \n"); 
            search_prefix(buffer);    
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
        history_head = add_to_history(history_head, input);
        exec_command(input);
    }

    free_history(history_head);
    return 0;
}
