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

struct background_job {
    int job_number;
    pid_t pid;
    char *command;
    int done;
    struct background_job *next;
};

struct background_job *bg_jobs = NULL;
int job_counter = 1;

void add_background_job(pid_t pid, char **cmd) {
    struct background_job *job = malloc(sizeof(struct background_job));
    job->job_number = job_counter++;
    job->pid = pid;

    // Concatenate all command arguments into a single string
    size_t cmd_len = 0;
    for (int i = 0; cmd[i] != NULL; i++) {
        cmd_len += strlen(cmd[i]) + 1; // +1 for space or null terminator
    }
    job->command = malloc(cmd_len + 2); // +2 for space and ampersand
    if (job->command) {
        job->command[0] = '\0';
        for (int i = 0; cmd[i] != NULL; i++) {
            strcat(job->command, cmd[i]);
            if (cmd[i + 1] != NULL) {
                strcat(job->command, " ");
            }
        }
        strcat(job->command, " &");
    }

    job->done = 0;
    job->next = bg_jobs;
    bg_jobs = job;
    printf("[%d] %d Running %s\n", job->job_number, job->pid, job->command);
}

void print_jobs() {
    // Count the number of jobs
    int job_count = 0;
    struct background_job *job = bg_jobs;
    while (job != NULL) {
        job_count++;
        job = job->next;
    }

    // Collect jobs into an array
    struct background_job **job_array = malloc(job_count * sizeof(struct background_job *));
    job = bg_jobs;
    for (int i = 0; i < job_count; i++) {
        job_array[i] = job;
        job = job->next;
    }

    // Sort the array by job number
    for (int i = 0; i < job_count - 1; i++) {
        for (int j = 0; j < job_count - i - 1; j++) {
            if (job_array[j]->job_number > job_array[j + 1]->job_number) {
                struct background_job *temp = job_array[j];
                job_array[j] = job_array[j + 1];
                job_array[j + 1] = temp;
            }
        }
    }

    // Print the jobs in sorted order
    for (int i = 0; i < job_count; i++) {
        job = job_array[i];
        if (job->done) {
            printf("[%d] Done    %s\n", job->job_number, job->command);
        } else {
            printf("[%d] %d Running %s\n", job->job_number, job->pid, job->command);
        }
    }

    // Remove and free done jobs
    struct background_job *prev = NULL;
    job = bg_jobs;
    while (job != NULL) {
        if (job->done) {
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
            prev = job;
            job = job->next;
        }
    }

    // Free the job array
    free(job_array);
}

void check_background_jobs() {
    struct background_job *job = bg_jobs;
    while (job != NULL) {
        int status;
        pid_t result = waitpid(job->pid, &status, WNOHANG);
        if (result == 0) {
            // Process is still running
            job = job->next;
        } else if (result == -1) {
            // Error occurred
            perror("waitpid");
            job = job->next;
        } else {
            // Process has finished
            job->done = 1;
            job = job->next;
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

void remove_done_jobs() {
    struct background_job *prev = NULL;
    struct background_job *job = bg_jobs;
    while (job != NULL) {
        if (job->done) {
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
            prev = job;
            job = job->next;
        }
    }
}

void terminate_background_jobs() {
    struct background_job *job = bg_jobs;
    while (job != NULL) {
        kill(job->pid, SIGTERM); // Send SIGTERM to terminate the background job
        job = job->next;
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
            remove_done_jobs();
            free(line);
            continue;
        }

         // Check if the command is "exit"
        if (strcmp(trimmed_line, "exit") == 0) {
            free(line);
            terminate_background_jobs(); // Terminate all background jobs
            break;
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

                if (background) {
                // Redirect stdout and stderr to /dev/null for background processes
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                }

                execvp(cmd[0], cmd);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                // This is the parent process
                setpgid(pid, pid);
                if (background) {
                    add_background_job(pid, cmd);
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