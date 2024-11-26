# Custom Shell with Command Autocomplete and History

This is a custom shell developed in C that includes features like command history, autocomplete, and system memory usage display. The shell allows users to execute commands, navigate directories, and use basic system commands. Additionally, it supports command history navigation (up and down arrows) and an autocomplete feature for command prefixes.

## Features

- **Command History**: Tracks previously entered commands and allows navigation through them using the up and down arrow keys.
- **Command Autocomplete**: Offers suggestions for commands based on a prefix when the user presses the `Tab` key.
- **System Memory Usage**: Displays the total and used physical memory when the `sysusage` command is executed.
- **Basic Shell Commands**: Supports basic commands like `cd`, `pwd`, `ls`, `exit`, and more.
- **Raw Mode Input**: Captures and processes user input in raw mode to handle backspace, autocomplete, and arrow keys.

## Dependencies

- GCC compiler for compiling C code.
- Standard C library headers (`stdio.h`, `stdlib.h`, `string.h`, etc.).

## Compilation Instructions

To compile and run the shell, follow these steps:

1. Clone or download the repository to your local machine.
2. Open a terminal in the project directory.
3. Run the following command to compile the program:
    ```bash
    gcc -o custom_shell custom_shell.c
    ```
4. After compilation, run the shell using:
    ```bash
    ./custom_shell
    ```

## Usage

### Shell Prompt

The shell prompt will display as:

username@system_name$


It provides the username and system name of the host.

### Common Commands

- `cd <directory>`: Change the current directory.
- `pwd`: Display the current working directory.
- `ls`: List files and directories.
- `whatisthis`: Command to question the sanity of the developer 
- `exit`: Exit the shell.
- `sysusage`: Display system memory usage.
- `whatisthis`: Display information about the shell.
- `help`: Display a list of available commands.

### Autocomplete

- Start typing a command and press `Tab` to autocomplete the command or see suggestions for possible commands that match the prefix.
- The autocomplete system is based on a simple trie (prefix tree) and supports common commands like `cd`, `ls`, `exit`, etc.

### Command History

- Use the **up arrow** to cycle through previously entered commands.
- Use the **down arrow** to move back to newer commands in the history.

### Example

```bash
username@system_name$ cd Documents
username@system_name$ pwd
/home/username/Documents
username@system_name$ ls
file1.txt  file2.txt  folder
username@system_name$ sysusage
Total Physical Memory: 8192.00 MB
Used Physical Memory: 4096.00 MB
username@system_name$ exit
```
### Acknowledgement

- Autocomplete feature is implemented using **Trie** data structure for faster searching of command prefix (also to brush up my DSA classes). It still needs some refining to be done as it may not work as intended. More features are yet to be added like CPU Usage, process commands, etc.


