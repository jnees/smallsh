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

    // Kill any remaining PIDs  TODO

    // exit the program.
    exit(0);
}

/**********************************************************************
    Function: status()
    Print either the exit status or the terminating signal of the last
    foreground process executed by the shell. If not foreground process
    has been run yet, returns 0. Does not consider cd, exit, or status
    as foreground processes.

    Args:
        None

    Returns:
        None: 

************************************************************************/ 
void getStatus(void){
    int status = 0;
    printf("Status: %d\n", status);
}

/**********************************************************************
    Function: main ()

************************************************************************/ 

int main(){
    char userInput[MAX_ARG_LEN + 1];
    int PIDS[MAX_PIDS] = { 0 };
    int cd;
    int exit;
    int status;

    // Main Prompt
    command_prompt:
    while(1){
        // Reset command struct
        Command cmd = {};
        cmd.background = false;

        // Prompt and get input
        printf(": ");
        fgets(userInput, MAX_ARG_LEN, stdin);
        
        // Trim string for training /n char
        userInput[strcspn(userInput, "\n")] = 0;

        // Reprompt if comment or blank input
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
        // CD
        cd = strncmp(cmd.args[0], "cd", 2);
        if(cd == 0 && strlen(cmd.args[0]) == 2){
            changeDir(&cmd);
        }

        // Exit
        exit = strncmp(cmd.args[0], "exit", 4);
        if(exit == 0 && strlen(cmd.args[0]) == 4){
            exitProgram(PIDS);
        }

        // Status -> TODO
        status = strncmp(cmd.args[0], "status", 6);
        if(status == 0 && strlen(cmd.args[0]) == 6){
            getStatus();
        }

        /*-------------------------------------------------------
         Handle other commands
         If the user enters a command other than cd, exit, or
         status, the shell will try to execute the command
         from the HOME directory in a new child process.
        --------------------------------------------------------*/

        // Other commands
        if (cd != 0 && exit != 0 && status != 0){
            // Fork child process
            int childExitStatus;
            pid_t childPid = fork();

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
                    
                    // printf("Arg[0]: %s\n", cmd.args[0]);
                    // printf("Arg[1]: %s\n", cmd.args[1]);
                    // printf("Arg[2]: %d\n", strcmp(cmd.args[2], ""));
                    
                    char *newargv[513] = {};
                    for (int i=0;i<513;i++){
                        if(strcmp(cmd.args[i], "") == 0){
                            newargv[i] = NULL;
                        } else {
                            newargv[i] = cmd.args[i];
                        }
                    }
                    newargv[513] = NULL;
                    execvp(newargv[0], newargv);
                    removePID(PIDS, childPid);

                    break;

                // Case -> Parent Process
                default:
                    // printPIDS(PIDS);
                    childPid = waitpid(-1, &childExitStatus, 0);
                    // printf("after: %d\n", childPid);

            }
        }


        // printPIDS(PIDS);

        // check child processes


        // TEST PID helpers
        // insertPID(PIDS, 14);
        // insertPID(PIDS, 24);
        // printPIDS(PIDS);
        // removePID(PIDS, 24);
        // printPIDS(PIDS);
    }
    
}