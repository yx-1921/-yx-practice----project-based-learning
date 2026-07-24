#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int yx_cd(char **args);
int yx_help(char **args);
int yx_exit(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[])(char **) = {
    &yx_cd,
    &yx_help,
    &yx_exit
};

int yx_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int yx_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int yx_help(char **args)
{
    int i;
    printf("bash learning!\n");
    for (int i = 0; i < yx_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }
    printf("select command to execute!\n");
    return 1;
}

int yx_exit(char **args)
{
    return 0;
}

int yx_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("lsh");
        // Error fork
    } else {
        // parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int yx_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        return 1;
    }
    // traverse built-in command
    for (int i = 0; i < yx_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return yx_launch(args);
}

char *yx_read_line(void)
{
#ifdef yx_USE_STD_GETLINE
    char *line = NULL;
    ssize_t bufsize = 0;
    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror("lsh: getline\n");
            exit(EXIT_FAILURE);
        }
    }
    return line;
#else
#define yx_RL_BUFFSIZE 1024
    int bufsize = yx_RL_BUFFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;
    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1) {
        c = getchar();
        if (c == EOF) {
            exit(EXIT_SUCCESS);
        } else if (c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += yx_RL_BUFFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
#endif
}

#define yx_TOK_BUFFSIZE 64
#define yx_TOK_DELIM " \t\r\n\a"
/*
    @brief split a lint into tokens
    @param line The line
    @return Null-terminated array of tokens.
*/
char **yx_split_line(char *line) 
{
    int bufsize = yx_TOK_BUFFSIZE;
    int position = 0;
    char **tokens = malloc(sizeof(char*) * bufsize);
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, yx_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += yx_TOK_BUFFSIZE;
            tokens_backup = tokens; // save old pointer
            tokens = realloc(tokens, sizeof(char*) * bufsize);
            if (tokens == NULL) {
                free(tokens_backup); // free old pointer
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, yx_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/*
    @brief loop getting input and execute it.
*/
void yx_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = yx_read_line();
        args = yx_split_line(line);
        status = yx_execute(args);

        free(line);
        free(args);

    }while (status);

}

/*
    @brief main entry point
    @param argc argument count
    @param argv argument vector
    @breturn status code
*/
int main(int argc, char **argv)
{
    yx_loop();

    return EXIT_SUCCESS;
}