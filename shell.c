#include "shell.h"
#include "task_scheduler.c"
#include "focus_mode.c"

Node *current = NULL;
Node *history_head;
TrieNode *autocomplete_head = NULL;
struct termios orig_termios;
struct sysinfo memInfo;
struct utsname unameData;
Job jobs[MAX_JOBS];
int job_count = 0;
char *common_commands[] = {
    "cd", "pwd", "ls", "exit", "clear", "echo", "help", "uname", "top", "whoami", "whatisthis",
    "kill", "service", "gcc", "bg", "fg",
    NULL};
/*
typedef struct {
    pid_t pid;
    int job_id;
    job_status status;
    int priority;
    char command[COMMAND_SIZE];
} Job;

*/
void add_job(pid_t pid, char **command, int priority)
{
    // jobs[job_count].command = strdup(command[0]);
    jobs[job_count].priority = priority;
    jobs[job_count].pid = pid;
    jobs[job_count].job_id = job_count + 1;
    jobs[job_count].status = RUNNING;
    job_count++;
}
void list_jobs()
{
    for (int i = 0; i < job_count; i++)
    {
        printf("[%d] %s (%d) %s\n", jobs[i].job_id, jobs[i].command, jobs[i].pid, jobs[i].status == RUNNING ? GRN "Running" RESET : RED "Stopped" RESET);
    }
}
void suspend_job(int job_id)
{
    pid_t pid = jobs[job_id - 1].pid;

    if (kill(pid, SIGTSTP) == -1)
    {
        perror("Failed to suspend job.");
        return;
    }
    jobs[job_id - 1].status = STOPPED;
    printf("Job %d (%s) suspended\n", job_id, jobs[job_id - 1].command);
}
void kill_job(int job_id)
{
    pid_t pid = jobs[job_id - 1].pid;

    if (kill(pid, SIGKILL) == -1)
    {
        perror("Failed to kill the job.");
        return;
    }
    jobs[job_id - 1].status = TERMINATED;
    printf("Job %d (%s) kill\n", job_id, jobs[job_id - 1].command);
}
void run_job_background(char **command, int priority)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Failed to fork.");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        // let the parent process take place too
        return;
    }
    else
    {
        add_job(pid, command, priority);
        execvp(command[0], command);
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    }
}

TrieNode *createNode()
{
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (!node)
    {
        perror("Memory error");
        exit(EXIT_FAILURE);
    }
    node->is_end = false;
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        node->children[i] = NULL;
    }
    return node;
}

void insert_into_trie(const char *command)
{
    if (!autocomplete_head)
    {
        autocomplete_head = createNode();
    }
    TrieNode *temp = autocomplete_head;
    while (*command)
    {
        int index = tolower(*command) - 'a';
        if (!temp->children[index])
        {
            temp->children[index] = createNode();
        }
        temp = temp->children[index];
        command++;
    }
    temp->is_end = true;
}

void collect_words(TrieNode *node, char *prefix, int length)
{
    if (node->is_end)
    {
        prefix[length] = '\0';
        printf("%s\n", prefix);
    }
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (node->children[i])
        {
            prefix[length] = 'a' + i;
            collect_words(node->children[i], prefix, length + 1);
        }
    }
}

void search_prefix(char *prefix)
{
    char *temp_string = strdup(prefix);
    if (!autocomplete_head)
        return;
    TrieNode *temp = autocomplete_head;
    int prefix_length = 0;
    while (*prefix)
    {
        int index = tolower(*prefix) - 'a';
        if (index < 0 || index >= ALPHABET_SIZE || !temp->children[index])
        {
            return;
        }
        temp = temp->children[index];
        prefix++;
        prefix_length++;
    }
    collect_words(temp, temp_string, prefix_length);
}

// Enable raw mode for terminal input
void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

// Disable raw mode and restore original terminal settings
void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// Add a command to the history list
Node *add_to_history(Node *head, const char *command)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL)
    {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->command, command);
    new_node->next = head;
    new_node->prev = NULL;
    if (head)
    {
        head->prev = new_node;
    }
    return new_node;
}

// Display the shell prompt
void prompt()
{
    char *user = getenv("USER");
    if (user != NULL)
    {
        printf("%s@", user);
    }
    else
    {
        printf("anonymous@");
    }
    if (uname(&unameData) == 0)
    {
        printf("%s$ ", unameData.sysname);
    }
    else
    {
        printf("unknownOS$ ");
    }
}

// Display system memory usage
void sysusage()
{
    sysinfo(&memInfo);
    long long totalPhysMem = memInfo.totalram;
    long long usedPhysMem = memInfo.totalram - memInfo.freeram;
    usedPhysMem *= memInfo.mem_unit;
    totalPhysMem *= memInfo.mem_unit;
    printf("Total Physical Memory: %.2f MB\n", totalPhysMem / (1024.0 * 1024.0));
    printf("Used Physical Memory: %.2f MB\n", usedPhysMem / (1024.0 * 1024.0));
}

void change_directory(char **args)
{
    if (args[1] == NULL)
    {
        printf("No directory provided.\n");
    }
    if (chdir(args[1]) != 0)
    {
        perror("Failed to change directory");
    }
    return;
}

void handle_redirection(char **args, char *file)
{
    int fd = 0;
    if (file)
    {
        fd = open(file, O_WRONLY | O_CREAT);
        if (fd < 0)
        {
            perror("Failed to open file for redirection");
            return;
        }
        else
        {
            int saved_stdout = dup(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);
            close(fd);

            // run the process
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("Failed to fork.");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                execvp(args[0], args);
                perror("Failed to execute command");
                exit(EXIT_FAILURE);
            }
            else
            {
                wait(NULL);
            }

            // restore stdout
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
    }
    else
    {
        printf("No file provided for redirection.\n");
        return;
    }
}

void handle_pipeline(char **command1, char **command2)
{
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1)
    {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }
    // pipefd[0] is the read end and the pipefd[1] is the write end

    pid1 = fork();
    if (pid1 == 0)
    {

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        execvp(command1[0], command1);
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0)
    {

        dup2(pipefd[0], STDIN_FILENO);
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

void handle_jobs(char **args, int arg_count)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "Failed to run background jobs: Require more arguements.\n");
        return;
    }
    int priority = atoi(args[arg_count - 1]);
    pid_t pid = fork();
}

void cleanup_old(char **args, int arg_count)
{
    if (arg_count < 4)
    {
        fprintf(stderr, "Failed to run cleanup command: Require more arguements.\n");
        return;
    }
    int size_in_mb = atoi(args[3]);
    char *path = strdup(args[1]);
    char *flag = strdup(args[2]);
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Failed to fork.");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        if (flag == "--size")
        {
            
        }
        else if (flag == "--old")
        {
            // Clean up old files based on size
        }
        else
        {
        }
    }
    else
        wait(NULL);
}
void handle_task_scheduler(char **arguments, int arg_count) {
    printf("Task Scheduler\n");
    if (arg_count < 3) {
        fprintf(stderr, "Failed to run task scheduler command: Requires more arguments.\n");
        return;
    }

    char *time_string = strdup(arguments[arg_count - 1]);
    time_t execution_delay;
    time_t current_time = time(NULL);

    switch (time_string[strlen(time_string) - 1]) {
        case 'm':
            time_string[strlen(time_string) - 1] = '\0';
            execution_delay = strtoll(time_string, NULL, 10) * 60;
            break;
        case 'h':
            time_string[strlen(time_string) - 1] = '\0';
            execution_delay = strtoll(time_string, NULL, 10) * 60 * 60;
            break;
        default:
            execution_delay = strtoll(time_string, NULL, 10);
            break;
    }

    free(time_string);

    time_t execution_time = current_time + execution_delay;

    char command[256] = {0};
    for (int i = 1; i < arg_count - 1; i++) {
        strcat(command, arguments[i]);
        if (i < arg_count - 2) {
            strcat(command, " ");
        }
    }
    printf("%s %lld\n", command, (long long int)execution_time - time(NULL));
    add_task(command, execution_time);
}

void handle_focus_mode(char **args, int arg_count) {
    if(arg_count < 1) {
        fprintf(stderr, "Not enough arguements, specify 'enable' or 'disable'\n");
        return;
    }
    
    if(strcmp(args[1], "enable") == 0) {
        enable_focus_mode();
    } else if(strcmp(args[1], "disable") == 0) {
        disable_focus_mode();
    } else {
        fprintf(stderr, "Invalid arguement, specify 'enable' or 'disable'\n");
        return;
    }
}

// Execute shell commands
void exec_command(char *input)
{
    bool redirection = false;
    bool pipeline = false;
    char *file = NULL;
    char *args[MAX_ARGUMENTS];
    char *command1[MAX_ARGUMENTS];
    char *command2[MAX_ARGUMENTS];
    int arg_count = 0;
    int command_two_idx = 0;
    char *token = strtok(input, " ");

    while (token != NULL)
    {
        // if(*token == '&') {
        //     // Handle background jobs
        //     args[arg_count] = NULL;
        //     token = strtok(NULL, " ");

        //     handle_jobs(args, arg_count);
        //     break;
        // }

        if (*token == '|')
        {
            pipeline = true;
            command1[arg_count] = NULL;
            token = strtok(NULL, " ");
            break;
        }

        if (*token == '>')
        {
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
    if (pipeline)
    {
        int i = 0;
        while (token != NULL)
        {
            command2[i++] = token;
            token = strtok(NULL, " ");
        }
        command2[i] = NULL;
        handle_pipeline(command1, command2);
        return;
    }
    // check for redirection
    if (redirection)
    {
        printf("Redirection detected.\n");
        handle_redirection(args, file);
        redirection = false;
        return;
    }
    if (strcmp(args[0], "whatisthis") == 0)
    {
        printf("This is a custom shell developed in C due to the fact that the developer was bored.\n");
        return;
    }
    if (strcmp(args[0], "help") == 0)
    {
        printf("Refer to https://github.com/SreehariSanjeev04/custom_shell for the commands. Use 'man' for more information regarding a command. \n");
        return;
    }
    if (strcmp(args[0], "exit") == 0)
    {
        printf("Exiting shell.\n");
        exit(EXIT_SUCCESS);
    }
    if (strcmp(args[0], "sysusage") == 0)
    {
        sysusage();
        return;
    }
    if (strcmp(args[0], "cd") == 0)
    {
        change_directory(args);
        return;
    }
    if(strcmp(args[0], "schedule") == 0)
    {
        handle_task_scheduler(args, arg_count);
        return;
    }
    if(strcmp(args[0], "focusmode") == 0) {
        handle_focus_mode(args, arg_count);
        return;
    }
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Failed to fork");
        return;
    }
    if (pid == 0)
    {
        execvp(args[0], args);
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    }
    else
    {
        wait(NULL);
    }
}

void readInput(char *buffer)
{
    int index = 0;
    char c;
    current = history_head;
    while (1)
    {
        c = getchar();
        if (c == '\n')
        {
            buffer[index] = '\0';
            printf("\n");
            break;
        }
        else if (c == 127)
        { // Backspace
            if (index > 0)
            {
                printf("\b \b");
                index--;
            }
        }
        else if (c == '\t')
        {
            buffer[index] = '\0';
            printf("\nSuggestions: \n");
            search_prefix(buffer);
            printf("\r\033[K");
            prompt();
            printf("%s", buffer);
        }
        else if (c == '\033')
        {
            getchar();
            switch (getchar())
            {
            case 'A': // Up arrow
                if (current && current->next)
                {
                    current = current->next;
                    index = strlen(current->command);
                    strcpy(buffer, current->command);
                    printf("\r\033[K");
                    prompt();
                    printf("%s", buffer);
                }
                break;
            case 'B': // Down arrow
                if (current && current->prev)
                {
                    current = current->prev;
                    index = strlen(current->command);
                    strcpy(buffer, current->command);
                    printf("\r\033[K");
                    prompt();
                    printf("%s", buffer);
                }
                break;
            }
        }
        else
        {
            if (index < MAX_INPUT - 1)
            {
                buffer[index++] = c;
                putchar(c);
            }
        }
    }
}

// Free history nodes
void free_history(Node *head)
{
    while (head)
    {
        Node *temp = head;
        head = head->next;
        free(temp);
    }
}

int main()
{
    char input[MAX_INPUT];
    history_head = NULL;

    atexit(disableRawMode);
    enableRawMode();

    // adding common commands to the trie
    int trie_index = 0;
    while (common_commands[trie_index] != NULL)
    {
        insert_into_trie(common_commands[trie_index++]);
    }

    while (1)
    {
        prompt();
        readInput(input);
        history_head = add_to_history(history_head, input);
        exec_command(input);
    }

    free_history(history_head);
    return 0;
}
