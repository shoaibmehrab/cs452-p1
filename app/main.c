#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include "../src/lab.h"

// Custom completion function
// char *command_generator(const char *text, int state) {
//     static int list_index, len;
//     static char **matches = NULL;
//     char *name;

//     // List of predefined commands for completion
//     static char *commands[] = {
//         "help",
//         "exit",
//         "ls",
//         "cd",
//         "history",
//         NULL
//     };

//     // Combine predefined commands and history entries
//     if (!state) {
//         list_index = 0;
//         len = strlen(text);

//         // Count history entries
//         int history_count = history_length;

//         // Allocate memory for matches
//         matches = (char **)malloc((history_count + 6) * sizeof(char *));
//         int match_index = 0;

//         // Add predefined commands to matches
//         for (int i = 0; commands[i] != NULL; i++) {
//             matches[match_index++] = commands[i];
//         }

//         // Add history entries to matches
//         for (int i = 0; i < history_count; i++) {
//             matches[match_index++] = history_get(i + 1)->line;
//         }

//         // Null-terminate the matches array
//         matches[match_index] = NULL;
//     }

//     // Return matches that start with the input text
//     while ((name = matches[list_index++])) {
//         if (strncmp(name, text, len) == 0) {
//             return strdup(name);
//         }
//     }

//     // Free allocated memory for matches
//     if (matches) {
//         free(matches);
//         matches = NULL;
//     }

//     return NULL;
// }

// Custom completion function
// char **command_completion(const char *text, int start, int end) {
//     rl_attempted_completion_over = 1;
//     return rl_completion_matches(text, command_generator);
// }

// Function to trim trailing spaces
// void trim_trailing_spaces(char *str) {
//     int len = strlen(str);
//     while (len > 0 && isspace((unsigned char)str[len - 1])) {
//         str[--len] = '\0';
//     }
// }

// Function to handle built-in commands
// int handle_builtin_commands(char *line, char **prompt) {
//     printf("Handling command: %s\n", line); // Debugging statement
//     if (strcmp(line, "exit") == 0) {
//         return 1; // Signal to exit the shell
//     } else if (strncmp(line, "cd", 2) == 0) {
//         char *dir = strtok(line + 2, " \t");
//         if (change_dir(&dir) != 0) {
//             fprintf(stderr, "cd: %s: %s\n", dir ? dir : "NULL", strerror(errno));
//         }
//         else {
//             free(*prompt); // Free the old prompt
//             *prompt = update_prompt(); // Update the prompt
//         }
//         return 0;
//     } else if (strcmp(line, "pwd") == 0) {
//         print_working_directory();
//         return 0;
//     }else if (strcmp(line, "history") == 0) {
//         HIST_ENTRY **hist_list = history_list();
//         if (hist_list) {
//             for (int i = 0; hist_list[i]; i++) {
//                 printf("%d: %s\n", i + history_base, hist_list[i]->line);
//             }
//         }
//         return 0;
//     }
//     return 0; // Not a built-in command
// }

struct background_job {
    int job_number;
    pid_t pid;
    char *command;
    int done;
    struct background_job *next;
};

struct background_job *bg_jobs = NULL;
int job_counter = 1;

void add_background_job(pid_t pid, char *command) {
    struct background_job *job = malloc(sizeof(struct background_job));
    job->job_number = job_counter++;
    job->pid = pid;
    job->command = strdup(command);
    job->done = 0;
    job->next = bg_jobs;
    bg_jobs = job;
    printf("[%d] %d %s\n", job->job_number, job->pid, job->command);
}

// void check_background_jobs() {
//     struct background_job *job = bg_jobs;
//     while (job != NULL) {
//         int status;
//         pid_t result = waitpid(job->pid, &status, WNOHANG);
//         if (result == 0) {
//             // Process is still running
//             job = job->next;
//         } else if (result == -1) {
//             // Error occurred
//             perror("waitpid");
//             job = job->next;
//         } else {
//             // Process has finished
//             job->done = 1;
//             job = job->next;
//         }
//     }
// }

void print_jobs() {
    struct background_job *job = bg_jobs;
    while (job != NULL) {
        if (job->done) {
            printf("[%d] Done    %s\n", job->job_number, job->command);
        } else {
            printf("[%d] %d Running %s\n", job->job_number, job->pid, job->command);
        }
        job = job->next;
    }
}

void check_background_jobs() {
    struct background_job *job = bg_jobs;
    struct background_job *prev = NULL;
    while (job != NULL) {
        int status;
        pid_t result = waitpid(job->pid, &status, WNOHANG);
        if (result == 0) {
            // Process is still running
            prev = job;
            job = job->next;
        } else if (result == -1) {
            // Error occurred
            perror("waitpid");
            if (prev == NULL) {
                bg_jobs = job->next;
            } else {
                prev->next = job->next;
            }
            free(job->command);
            struct background_job *temp = job;
            job = job->next;
            free(temp);
        } else {
            // Process has finished
            printf("[%d] Done %s\n", job->job_number, job->command);
            if (prev == NULL) {
                bg_jobs = job->next;
            } else {
                prev->next = job->next;
            }
            free(job->command);
            struct background_job *temp = job;
            job = job->next;
            free(temp);
        }
    }
}

void free_background_jobs() {
    struct background_job *job = bg_jobs;
    while (job != NULL) {
        struct background_job *next = job->next;
        free(job->command);
        free(job);
        job = next;
    }
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

     // Ignore specific signals in the parent process
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    // Initialize the shell
    struct shell sh;
    sh_init(&sh);

    //Initialize history
    using_history();

    // Get maximum argument length
    long arg_max = sysconf(_SC_ARG_MAX);
    if (arg_max == -1) {
        perror("sysconf");
        exit(EXIT_FAILURE);
    }

    // Main loop for the shell
    while (1) {

        // Check for finished background jobs
        check_background_jobs();

        char *line = readline(sh.prompt);
        if (!line) {
             // Handle EOF or read error
            if (feof(stdin)) {
                printf("\n");
                break;
            }
            continue;
        }

        // Trim leading and trailing whitespace
        char *trimmed_line = trim_white(line);

        // Skip empty commands
        if (*trimmed_line == '\0') {
            free(line);
            continue;
        }

        if (*trimmed_line) {
            add_history(trimmed_line);
        }

         // Check if the command is "jobs"
        if (strcmp(trimmed_line, "jobs") == 0) {
            print_jobs();
            free(line);
            continue;
        }

        // Check if the command should run in the background
        int background = 0;
        size_t len = strlen(trimmed_line);
        if (len > 0 && trimmed_line[len - 1] == '&') {
            background = 1;
            trimmed_line[len - 1] = '\0'; // Remove the ampersand
            trimmed_line = trim_white(trimmed_line); // Trim any trailing whitespace
        }

        // Parse the command line
        char **cmd = malloc(arg_max * sizeof(char *));
        if (cmd == NULL) {
            perror("malloc");
            free(line);
            continue;
        }

        char *token;
        int argc = 0;

        token = strtok(trimmed_line, " ");
        while (token != NULL && argc < arg_max - 1) {
            cmd[argc++] = strdup(token);
            token = strtok(NULL, " ");
        }
        cmd[argc] = NULL;

        if (cmd[0] && !do_builtin(&sh, cmd)) {
            // Execute external command
            pid_t pid = fork();
            if (pid == 0) {
                // This is the child process
                pid_t child = getpid();
                setpgid(child, child);

                // Set signals back to default in the child process
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);

                execvp(cmd[0], cmd);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                // This is the parent process
                setpgid(pid, pid);
                if (background) {
                    add_background_job(pid, line);
                } else {
                    tcsetpgrp(STDIN_FILENO, pid);

                    // Wait for the child process to finish
                    int status;
                    waitpid(pid, &status, WUNTRACED);

                    // Restore the shell as the foreground process group
                    tcsetpgrp(STDIN_FILENO, getpgrp());
                }
            } else {
                perror("fork");
            }
        }

        // Free the allocated memory for cmd
        for (int i = 0; i < argc; i++) {
            free(cmd[i]);
        }

        free(cmd);
        free(line);
    }

    free_background_jobs();
    sh_destroy(&sh);
    return 0;
}