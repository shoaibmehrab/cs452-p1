#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include "../src/lab.h"

// Custom completion function
char *command_generator(const char *text, int state) {
    static int list_index, len;
    static char **matches = NULL;
    char *name;

    // List of predefined commands for completion
    static char *commands[] = {
        "help",
        "exit",
        "ls",
        "cd",
        "history",
        NULL
    };

    // Combine predefined commands and history entries
    if (!state) {
        list_index = 0;
        len = strlen(text);

        // Count history entries
        int history_count = history_length;

        // Allocate memory for matches
        matches = (char **)malloc((history_count + 6) * sizeof(char *));
        int match_index = 0;

        // Add predefined commands to matches
        for (int i = 0; commands[i] != NULL; i++) {
            matches[match_index++] = commands[i];
        }

        // Add history entries to matches
        for (int i = 0; i < history_count; i++) {
            matches[match_index++] = history_get(i + 1)->line;
        }

        // Null-terminate the matches array
        matches[match_index] = NULL;
    }

    // Return matches that start with the input text
    while ((name = matches[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    // Free allocated memory for matches
    if (matches) {
        free(matches);
        matches = NULL;
    }

    return NULL;
}

// Custom completion function
char **command_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_generator);
}

// Function to trim trailing spaces
void trim_trailing_spaces(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

// Function to handle built-in commands
int handle_builtin_commands(char *line) {
    printf("Handling command: %s\n", line); // Debugging statement
    if (strcmp(line, "exit") == 0) {
        return 1; // Signal to exit the shell
    } else if (strncmp(line, "cd", 2) == 0) {
        char *dir = strtok(line + 2, " \t");
        if (change_dir(&dir) != 0) {
            fprintf(stderr, "cd: %s: %s\n", dir ? dir : "NULL", strerror(errno));
        }
        return 0;
    } else if (strcmp(line, "history") == 0) {
        HIST_ENTRY **hist_list = history_list();
        if (hist_list) {
            for (int i = 0; hist_list[i]; i++) {
                printf("%d: %s\n", i + history_base, hist_list[i]->line);
            }
        }
        return 0;
    }
    return 0; // Not a built-in command
}

int main(int argc, char **argv)
{
    int opt;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                // Print version and exit
                printf("Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                return 0;
            default:
                fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Initialize history
    using_history();

    // Set custom completion function
    rl_attempted_completion_function = command_completion;

    //Get the custom prompt from the environment variable
    // char *prompt_env = getenv("MY_PROMPT");
    // char prompt[256];
    // if (prompt_env != NULL) {
    //     snprintf(prompt, sizeof(prompt), "%s> ", prompt_env);
    // } else {
    //     snprintf(prompt, sizeof(prompt), "$ "); // Default prompt
    // }

    char *prompt = get_prompt("MY_PROMPT");
    if (prompt == NULL) {
        fprintf(stderr, "Failed to allocate memory for prompt\n");
        exit(EXIT_FAILURE);
    }

    // Read user input in a loop
    char *line;
    while ((line = readline(prompt))) {
        // Trim trailing spaces
        trim_trailing_spaces(line);

        // Check for built-in commands
        if (handle_builtin_commands(line)) {
            free(line);
            break;
        }

        // Add input to history
        add_history(line);

        // Free the allocated line
        free(line);
    }

    free(prompt);

    return 0;
}