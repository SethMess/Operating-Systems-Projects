#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024        // buffer size for recieving commands
#define SMALLER_BUFFER_SIZE 200 // buffer size for getting user information

// node struct for background processes linked list
struct bg_pro
{
    pid_t pid;
    char command[BUFFER_SIZE];
    struct bg_pro *next;
};

// tokenizes the users input from line and stores them in args
void get_user_input(char **args, char *line)
{
    args[0] = strtok(line, " \n");
    int i = 0;
    while (args[i] != NULL)
    {
        args[i + 1] = strtok(NULL, " \n");
        i++;
    }
    args[i + 1] = NULL;
}

// changes the current directory to the path contained in args
void change_directory(char **args)
{
    char dir[BUFFER_SIZE];
    if (args[1] == NULL || strncmp(args[1], "~", 1) == 0) // checks if cd was given no input or a ~
    {
        strcpy(dir, getenv("HOME")); // gets the home environment variable
        if (args[1] != NULL)
        {
            strncat(dir, &args[1][1], strlen(&args[1][1]));
        }
    }
    else
    {
        strncpy(dir, args[1], strlen(args[1]));
    }
    int dir_result = chdir(dir);
    if (dir_result != 0)
    {
        printf("ERROR: No such file or directory\n");
    }
}

// this function prints out all currently running background processes
void bg_list(struct bg_pro *root, int num_bg_pro)
{
    struct bg_pro *cur = root;
    while (cur != NULL)
    {
        printf("%d: %s\n", cur->pid, cur->command);
        cur = cur->next;
    }
    printf("Total Background jobs: %d\n", num_bg_pro);
}

/* this function forks a child replaces it using execvp and stores it into a linked list and it doesnt wait for the
child to return before contining letting it run in the background*/
void start_background_processes(char **args, struct bg_pro **root, int *num_bg_pro)
{
    int p = fork();
    if (p == 0)
    {
        execvp(args[1], &args[1]);
        printf("ERROR: invalid command\n");
        exit(1);
    }
    else if (p > 0)
    {
        // initializing the new node
        struct bg_pro *new_pro = malloc(sizeof(struct bg_pro));
        if (new_pro == NULL)
        {
            printf("ERROR malloc failed\n");
            exit(1);
        }
        // adds new node to linked list
        new_pro->pid = p;
        new_pro->next = NULL;
        int i = 1;
        while (args[i] != NULL)
        {
            if (i == 1)
            {
                strncpy(new_pro->command, args[i], strlen(args[i]) + 1);
                strcat(new_pro->command, " ");
            }
            else
            {
                strncat(new_pro->command, args[i], strlen(args[i]) + 1);
                strcat(new_pro->command, " ");
            }
            i++;
        }

        if (*num_bg_pro == 0)
        {
            *root = new_pro;
        }
        else
        {
            struct bg_pro *cur = *root; // adds current process to the end of the linked list
            while (cur->next != NULL)
            {
                cur = cur->next;
            }
            cur->next = new_pro;
        }
        (*num_bg_pro)++;
    }
    else // Pid less than 0
    {
        printf("ERROR: Child Process could not be created\n");
        exit(1);
    }
}

// this function removes and prints out any background processes that have finished executing
void end_background_processes(struct bg_pro **root, int *num_bg_pro)
{
    struct bg_pro *cur = *root;
    struct bg_pro *prev = NULL;
    while (cur != NULL)
    {
        pid_t result_pid = waitpid(cur->pid, NULL, WNOHANG);
        if (result_pid == -1)
        {
            printf("ERROR background process failed\n");
            exit(1);
        }
        else if (result_pid == 0)
        {
            prev = cur;
            cur = cur->next;
        }
        else
        {
            if (prev == NULL)
            {
                *root = cur->next;
                printf("%d: %s has terminated\n", cur->pid, cur->command);
                free(cur);
                cur = *root;
            }
            else
            {
                prev->next = cur->next;
                printf("%d: %s has terminated\n", cur->pid, cur->command);
                free(cur);
            }
            (*num_bg_pro)--;
        }
    }
}

// this function forks a child process then replaces it using execvp to whatever command is passed in args
void normal_process(char **args)
{
    int p = fork();
    if (p == 0)
    {
        execvp(args[0], args);
        printf("ERROR: invalid command\n");
        exit(1);
    }
    else if (p > 0)
    {
        pid_t pid = waitpid(p, NULL, WUNTRACED);
    }
    else
    {
        printf("ERROR: Child Process could not be created\n");
        exit(1);
    }
}

int main()
{
    // initialize buffers
    char hostname[SMALLER_BUFFER_SIZE];
    char directory[SMALLER_BUFFER_SIZE];
    char line_start[BUFFER_SIZE];

    // gets information about the user and directory to print each iteration
    char *username = getlogin();
    gethostname(hostname, sizeof(hostname));
    getcwd(directory, sizeof(directory));

    // formats the information
    sprintf(line_start, "%s@%s: %s > ", username, hostname, directory);
    const char *prompt = line_start;

    // Background process initalization
    struct bg_pro *root = NULL; // head pointer to linked list
    int num_bg_pro = 0;         // number of background processes running

    int bailout = 0; // loop termination variable
    while (!bailout) // main loop that runs while the shell is running
    {
        usleep(10000); // this line of code waits for any current command to print before continuing with the loop such as "bg ls"
        // updates information about user and current directory
        gethostname(hostname, sizeof(hostname));
        getcwd(directory, sizeof(directory));
        sprintf(line_start, "%s@%s: %s > ", username, hostname, directory);

        char *line = readline(prompt); // gets user input

        // tokenizes the user input
        char *args[BUFFER_SIZE];
        get_user_input(args, line);
        if (strcmp(args[0], "exit") == 0) // checks to see if the program should be exited
        {
            bailout = 1;
        }
        else // continues with normal excecution
        {
            if (strcmp(args[0], "cd") == 0) // changing directories
            {
                change_directory(args);
            }
            else if (strcmp(args[0], "bg") == 0) // background processes
            {
                start_background_processes(args, &root, &num_bg_pro);
            }
            else if (strcmp(args[0], "bglist") == 0) // background process list
            {
                bg_list(root, num_bg_pro);
            }
            else // normal processes
            {
                normal_process(args);
            }
            if (num_bg_pro > 0) // checks to see if background processes have finished
            {
                end_background_processes(&root, &num_bg_pro);
            }
        }

        free(line); // frees memory used to get user input
    }
}