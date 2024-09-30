/**Update this file with the starter code**/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <linux/limits.h>
#include "lab.h"

char *get_prompt(const char *env) {
    const char *prompt_env = getenv(env);
    const char *default_prompt = "shell>";
    const char *prompt = prompt_env ? prompt_env : default_prompt;
    char *result = malloc(strlen(prompt) + 1);
    if (result) {
        strcpy(result, prompt);
    }
    return result;
}

int change_dir(char **dir) {
    char *path;

    if (dir[1] == NULL) {
        // No directory specified, use HOME
        path = getenv("HOME");
        if (path == NULL) {
            // HOME not set, use getpwuid
            struct passwd *pw = getpwuid(getuid());
            if (pw == NULL) {
                return -1; // Error, could not determine home directory
            }
            path = pw->pw_dir;
        }
    } else {
        path = dir[1];
    }

    return chdir(path);
}

// Function to parse the command line input
char **cmd_parse(const char *line) {

    size_t arg_max = sysconf(_SC_ARG_MAX);
    char **args = malloc(arg_max * sizeof(char *));
    if (!args) return NULL;

    char *token;
    char *line_copy = strdup(line);
    if (!line_copy) {
        free(args);
        return NULL;
    }
    size_t index = 0;

    token = strtok(line_copy, " ");
    while (token != NULL && index < arg_max - 1) {
        args[index] = strdup(token);
        if (!args[index]) {
            // Free previously allocated memory in case of failure
            for (size_t i = 0; i < index; i++) {
                free(args[i]);
            }
            free(args);
            free(line_copy);
            return NULL;
        }
        index++;
        token = strtok(NULL, " ");
    }
    args[index] = NULL;

    free(line_copy);
    return args;
}

// Function to free the parsed command line input
void cmd_free(char **line) {
    if (!line) return;
    for (size_t i = 0; line[i] != NULL; i++) {
        free(line[i]);
    }
    free(line);
}

// Function to trim whitespace from the start and end of a string
char *trim_white(char *line) {
    if (!line) return NULL;

    char *start = line;

    // Trim leading whitespace
    while (isspace((unsigned char)*start)) start++;

    // Trim trailing whitespace
    if (*start) {
        char *end = start + strlen(start) - 1;
        while (end > start && isspace((unsigned char)*end)) end--;
        end[1] = '\0';
    }

    return start;
}

bool do_builtin(struct shell *sh, char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        sh_destroy(sh);
        exit(0);
    } else if (strcmp(argv[0], "cd") == 0) {
        const char *path = NULL;
        if (argv[1] == NULL) {
            path = getenv("HOME");
            if (path == NULL) {
                struct passwd *pw = getpwuid(getuid());
                if (pw != NULL) {
                    path = pw->pw_dir;
                }
            }
        } else {
            path = argv[1];
        }

        if (path == NULL || chdir(path) != 0) {
            perror("cd");
        }
        return true;
    } else if (strcmp(argv[0], "history") == 0) {
        HIST_ENTRY **history_list_vm = history_list();
        if (history_list_vm) {
            for (int i = 0; history_list_vm[i]; i++) {
                printf("%d: %s\n", i + history_base, history_list_vm[i]->line);
            }
        }
        return true;
    }
    return false;
}


void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) {
            kill(-sh->shell_pgid, SIGTTIN);
        }

        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }

    sh->prompt = get_prompt("MY_PROMPT");
}

void sh_destroy(struct shell *sh) {
    free(sh->prompt);
}

void parse_args(int argc, char **argv) {
    // Implement argument parsing if needed
    // For now, we just print the arguments
    for (int i = 0; i < argc; i++) {
        printf("Arg %d: %s\n", i, argv[i]);
    }
}

void print_working_directory() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
}