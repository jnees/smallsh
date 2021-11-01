#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

#define MAX_ARGS 512
#define MAX_PIDS 512
#define MAX_ARG_LEN 2048
#define MAX_PID_LEN 6


typedef struct {
    char args[MAX_ARGS + 1][MAX_ARG_LEN + 1];
    char redir_path_in[MAX_ARG_LEN + 1];
    char redir_path_out[MAX_ARG_LEN + 1];
    bool background;
} Command;


/**********************************************************************
    Function: parseCommand(userInput string, Command *cmd):

    Parses the user's command line input into a commend struct. Handles
    variable expansion in the case that an arg contains "$$". Arg
    will have current PID inserted in place of "$$" in the string.

    Example: (Assume PID = 917)
        smallsh$$ => smallsh917
        $$$exampl$$e => 917$exampl917e
    
    Args:
        userInput: input that has been prescreened for comment/null inputs.
        *cmd: Pointer to the Command struct that is being populated.
    
    Returns:
        None

************************************************************************/ 
void parseCommand(char * input, Command *cmd){

    int pid = getpid();
    char* pidstr = NULL;
    char* temp = NULL;
    temp = malloc(MAX_ARG_LEN);

    // Get process id string to insert into variable expansions
    pidstr = malloc(MAX_PID_LEN + 1);
    sprintf(pidstr, "%d", pid);

    // Parse Commands from userInput into struct
    char *token = NULL;
    token = strtok(input, " ");

    int argNumber = 0;

    while (token != NULL){

        switch(token[0]){
            
            // Case token is ">" -> Next token is output redirect filepath
            case '>':
                // Get next token and save to struct
                token = strtok(NULL, " ");
                strcpy(cmd->redir_path_out, token);
                break;

            // Case token is "<" -> Next token is input redirect filepath
            case '<':
                // Get next token and save to command struct
                token = strtok(NULL, " ");
                strcpy(cmd->redir_path_in, token);
                break;

            // case token is "&" -> Process will run in background
            case '&':
                cmd->background = true;
                break;

            // case token is all other words -> Copy to args and exapand variable if necessary.
            default:

                /************************************************
                   Variable Expansion
                *************************************************/

                // Case -> No expansion needed
                if (!strstr(token, "$$")){
                    strcpy(cmd->args[argNumber], token);
                    argNumber++;
                    break;
                }

                // Case -> String contains $$ -> Replace with PID
                int temp_pos = 0;
                for (int i=0; token[i] != '\0'; i++){

                    // Case -> Pointer is at beginning of $$
                    if (token[i] == '$' && token[i + 1] == '$'){
                        strcpy(temp + temp_pos, pidstr);
                        temp_pos += strlen(pidstr);
                        temp[temp_pos] = '\0';
                        // Skip forward one in the loop to pass the next $
                        i += 1;

                    // Case -> Pointer is not at the end of a $$
                    } else {
                        strcpy(temp + temp_pos, &token[i]);
                        temp_pos += 1;
                        temp[temp_pos] = '\0';
                    }
                }

                // Add expanded string to args
                strcpy(cmd->args[argNumber], temp);
                argNumber++;
        }

        // Move to next argument
        token = strtok(NULL, " "); 
    }

    strcpy(cmd->args[argNumber + 1], "\0");

    free(pidstr);
    free(temp);
}

/**********************************************************************
    Function: printCWD
    Helper function to display the CWD for debugging
************************************************************************/
void printCWD(void){
    char *buf=getcwd(NULL,0);
    printf("CWD: %s\n", buf);
    free(buf);
}

/**********************************************************************
 * insertPID(*PIDS, pid)
 * Add a new pid to the PIDS array
**********************************************************************/
void insertPID(int *PIDS, int pid){

    // Loop through array, set value to 0 if found
    for(int i = 0; i < MAX_PIDS; i++){
        if (PIDS[i] == 0){
            PIDS[i] = pid;
            return;
        }
    }
}

/**********************************************************************
 * printPIDS(*PIDS, pid)
 * Print the list of PIDs to the standard out. For Debugging.
**********************************************************************/
void printPIDS(int *PIDS){
    printf("PID LIST: \n");
    for(int i = 0; i < MAX_PIDS; i++){
        if (PIDS[i] != 0){
            printf("PIDS[%d]: %d\n", i, PIDS[i]);
        }
    }
    printf("--END PID LIST--\n");
}


/**********************************************************************
  removePID(*PIDS, pid)
  Remove a given pid from the PIDS array. Does nothing if pid is
  not found in PIDS array.
 
  Args:
    *PIDS - pointer to process ids array.
    pid - (int) process id.

  Returns:
    none
**********************************************************************/
void removePID(int *PIDS, int pid){
    for(int i = 0; i < MAX_PIDS; i++){
        if (PIDS[i] == pid){
            PIDS[i] = 0;
            return;
        }
    }
}

/**********************************************************************
    Function: cd(Command *cmd)
    Change the current working directory of smallsh.

    Args:
        cmd: pointer to parsed command struct.

        path (CMD->args[1]): if specified as the first arg after cd in user 
        input, change to this directory path. If cd is called without other
        arguments, it will change to the path specified in the HOME
        env variable.

    Returns:
        None.
    
    Changes:
        PWD to HOME or path found in CMD->args[1]

************************************************************************/
void changeDir(Command *cmd){

    char* homepath;

    // Case -> cd without path: Change to HOME
    if(strlen(cmd->args[1]) == 0){
        homepath = getenv("HOME");
        chdir(homepath);
        return;
    } 
    
    // Case -> cd with path: change to path
    chdir(cmd->args[1]);
    return;
}
    
/**********************************************************************
    Function: exitProgram(*PIDS)

    Exits the program and any child processes that may still be running.

    Args:
        Pointer to list of process ids
************************************************************************/ 
int exitProgram(int *PIDs){

    // wait for remaining children and exit.
    while (1){
        int res = wait(NULL);
        if (res == -1){
            printf("Exiting...\n");
            exit(0);
        } else {
            printf("Process %d finished...\n", res);
        }
    }
    
}

/**********************************************************************
    Function: main ()

************************************************************************/ 

int main(){
    char userInput[MAX_ARG_LEN + 1];
    int PIDS[MAX_PIDS] = { 0 };
    int cd;
    int exit_command;
    int status;
    pid_t childPid = 0;
    int childExitStatus = 0;


    // Main Prompt
    command_prompt:
    while(1){
    
        /**********************************************************
         Reset command struct -> 
         This program reuses the commend struct for each new 
         command in the process.
        ***********************************************************/
        Command cmd = {};

        // Clear in/out redirect paths
        cmd.background = false;
        for(int i = 0; cmd.redir_path_out[i] != 0; i++){
            cmd.redir_path_out[i] = 0;
        }

        for(int i = 0; cmd.redir_path_in[i] != 0; i++){
            cmd.redir_path_out[i] = 0;
        }
        
        // Prompt and get new command input
        printf(": ");
        fflush(stdout);
        fgets(userInput, MAX_ARG_LEN, stdin);
        
        // Trim string for training /n char
        userInput[strcspn(userInput, "\n")] = 0;

        // Reprompt if comment or blank input, else parse the new command.
        if ((userInput[0] == '#') | (userInput[0] == '\n')) {
            goto command_prompt;
        } else {
            parseCommand(userInput, &cmd);
            // Debugging
            // printf("arg 0: %s\n", cmd.args[0]);
            // printf("arg 1: %s\n", cmd.args[1]);
            // printf("arg 2: %s\n", cmd.args[2]); 
        }

        /*-------------------------------------------------------
         Handle built in commands
           cd, exit, and status
        --------------------------------------------------------*/
        // CD -> Change directories. Default is HOME
        cd = strncmp(cmd.args[0], "cd", 2);
        if(cd == 0 && strlen(cmd.args[0]) == 2){
            changeDir(&cmd);
        }

        // Exit -> Wait for child processes and exit.
        exit_command = strncmp(cmd.args[0], "exit", 4);
        if(exit_command == 0 && strlen(cmd.args[0]) == 4){
            exitProgram(PIDS);
        }

        // Status -> Prints status of child process.
        status = strncmp(cmd.args[0], "status", 6);
        if(status == 0 && strlen(cmd.args[0]) == 6){
            // Case -> Child has exited
            if(WIFEXITED(childExitStatus)){
                printf("Process %d exited. Status = %d\n", childPid, WEXITSTATUS(childExitStatus));
            } else if (WIFSIGNALED(childExitStatus)){
                printf("The processed received a signal: %d\n", WTERMSIG(childExitStatus));
            } else if (WIFSTOPPED(childExitStatus)){
                printf("The process was stopped by signal %d\n", WSTOPSIG(childExitStatus));
            } else if(WIFCONTINUED(childExitStatus)){
                printf("Process is resumed.\n");
            }
        }

        /*-------------------------------------------------------
         Handle other commands
         If the user enters a command other than cd, exit, or
         status, the shell will try to execute the command
         from the HOME directory in a new child process.
        --------------------------------------------------------*/

        // Other commands
        if (cd != 0 && exit_command != 0 && status != 0){
            // Fork child process
            childPid = fork();

            // Add child process to process list.
            insertPID(PIDS, childPid);

            switch(childPid){

                // Case -> Fork error
                case -1:
                    perror("fork() failed.");
                    // exit(1);
                    break;

                // Case -> Child Process
                case 0:;
                    // Create list of pointers to arg strings with Null terminator at end.
                    char *newargv[513] = {};
                    for (int i=0;i<513;i++){
                        if(strcmp(cmd.args[i], "") == 0){
                            newargv[i] = NULL;
                        } else {
                            newargv[i] = cmd.args[i];
                        }
                    }
                    newargv[512] = NULL;

                    /***************************************
                     Handle Redirects
                    ****************************************/
                   int targetFD;
                   int sourceFD;
                   int result;

                    // Case -> Redirect Input found in command.
                    if(cmd.redir_path_in[0] != 0){
                        // Open source file if found
                        sourceFD = open(cmd.redir_path_in, O_RDONLY);
                        if (sourceFD == -1) { 
                            perror("source open()"); 
                            exit(1); 
	                    }

                        // Redirect the Input stream (0) to the sourceFD
                        result = dup2(sourceFD, 0);
                        if (result == -1){
                            perror("target dup2() - Source");
                            exit(2);
                        }

                    }

                    // Handle Redirect Output if needed
                    if(cmd.redir_path_out[0] != 0){
                        // Create or open output file specified by redirect out path.
                        targetFD = open(cmd.redir_path_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (targetFD == -1) { 
                            perror("target open()"); 
                            exit(1); 
                        } else {
                            printf("File sucessfully made.");
                        }

                        // Redirect stdout (stream 1) to the redirect out file.
                        result = dup2(targetFD, 1);
                        if (result == -1){
                            perror("target dup2() - Destination");
                            exit(2);
                        }
                    }

                    // Execute the command
                    execvp(newargv[0], newargv);
                    perror("Execvp");

                // Case -> Parent Process
                default:
                    // sleep(1);
                    // Monitor child process
                    waitpid(0, &childExitStatus, WNOHANG);
                    break;
            }
        }
    }
    
    
}