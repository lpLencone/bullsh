#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define loop while (true)
#define panic() do { fprintf(stderr, "%s:%d in %s: %s\n", __FILE__, __LINE__, __FUNCTION__, strerror(errno)); exit(1); } while (0)
#define panic_if(cond) do { if (cond) panic(); } while (0)

#define BULLSH_RL_BUFSIZE 1024
char *bullsh_read_line(void)
{
    int bufsize = BULLSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(bufsize);
    int c;

    if (buffer == NULL) {
        panic();
    }

    loop {
        c = getchar();
        
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += BULLSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (buffer == NULL) {
                panic();
            }
        }
    }
}

#define BULLSH_TOK_BUFSIZE 64
#define BULLSH_TOK_DELIM " \t\r\b\a"
char **bullsh_split_line(char *line)
{
    int bufsize = BULLSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    panic_if(tokens == NULL);

    char *save_ptr = NULL;
    token = strtok_r(line, BULLSH_TOK_DELIM, &save_ptr);
    while (token != NULL) {
        tokens[position++] = token;

        if (position >= bufsize) {
            bufsize += BULLSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            panic_if(tokens == NULL);
        }

        token = strtok_r(save_ptr, BULLSH_TOK_DELIM, &save_ptr);
    }
    tokens[position] = NULL;
    return tokens;
}

int bullsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    panic_if((pid = fork()) < 0);

    if (pid == 0) {
        panic_if(execvp(args[0], args) == -1);
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/*
 * Function Declarations for builtin shell commands:
 */
int bullsh_cd(char **args);
int bullsh_help(char **args);
int bullsh_exit(char **args);

/*
 * List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &bullsh_cd,
    &bullsh_help,
    &bullsh_exit
};

#define BULLSH_NUM_BUILTINS sizeof(builtin_str) / sizeof(char *)

/*
 * Builtin function implementations
 */

int bullsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "bullsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            fprintf(stderr, "bullsh_cd %s\n", strerror(errno));
        }
    }
    return 1;
}

int bullsh_help(char **args)
{
    (void) args;

    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (int i = 0; i < BULLSH_NUM_BUILTINS; i++) {
        printf("    %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int bullsh_exit(char **args)
{
    (void) args;
    return 0;
}

int bullsh_execute(char **args)
{
    if (args[0] == NULL) {
        // Empty command
        return 1;
    }

    for (int i = 0; i < BULLSH_NUM_BUILTINS; i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return bullsh_launch(args);
}

void bullsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = bullsh_read_line();
        args = bullsh_split_line(line);
        status = bullsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(void)
{
    bullsh_loop();

    return 0;
}
