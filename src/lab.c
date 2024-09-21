/**Update this file with the starter code**/

#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include "lab.h"

char *get_prompt(const char *env) {
    const char *prompt_env = getenv(env);
    const char *default_prompt = "shell> ";
    const char *prompt = prompt_env ? prompt_env : default_prompt;
    char *result = malloc(strlen(prompt) + 1);
    if (result) {
        strcpy(result, prompt);
    }
    return result;
}

int change_dir(char **dir) {
    if (dir == NULL || *dir == NULL) {
        // No directory specified, use HOME
        *dir = getenv("HOME");
        if (*dir == NULL) {
            // HOME not set, use getpwuid
            struct passwd *pw = getpwuid(getuid());
            if (pw == NULL) {
                return -1; // Error, could not determine home directory
            }
            *dir = pw->pw_dir;
        }
    }
    return chdir(*dir);
}