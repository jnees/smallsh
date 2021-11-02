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
#include <signal.h>

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

int foreground_only_mode = 0;


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

                /*------------------------------------------------
                   Variable Expansion:

                   Find and replace "$$" in the argument with the 
                   current process id.
                -------------------------------------------------*/

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

/***********************************************************************
 Function: handle_SIGTSTP()
 
 Displays message and toggles foreground_only_mode, which causes
 the shell to toggle foreground_only_mode, which will ignore 
 any & symbols found in subsequent user commands.
 ************************************************************************/ 
void handle_SIGTSTP(int signo){
    char* message_on = "Entering foreground-only mode (& is now ignored)\n:\0";
    char* message_off = "Foreground-only mode off (& no longer ignored)\n:\0";

    // Toggle foreground_only_mode
    foreground_only_mode = !foreground_only_mode;

    // Send appropriate message
    foreground_only_mode ? write(STDOUT_FILENO, message_on, 53): 
                            write(STDOUT_FILENO, message_off, 51);
    return;
}


/**********************************************************************
    Function: main ()
    The main shell function. Continuously prompts a user for shell
    commands and executes them until the user types the exit command.
************************************************************************/ 

int main(){
    char userInput[MAX_ARG_LEN + 1];
    int PIDS[MAX_PIDS] = { 0 };
    int cd;
    int exit_command;
    int status;
    pid_t childPid = 0;
    int wstatus = 0;
    int lastForegroundPID = 0;
    int lastForegroundStatus = 0;


    /*----------------------------------------------------
     Signal Handling
    -----------------------------------------------------*/
    
    // Parent ignores SIGINT
    struct sigaction SIGINT_action = {};
	SIGINT_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Parent uses custom handler for SIGTSTP
    struct sigaction SIGTSTP_action = {};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);


    /*----------------------------------------------------
     Main Prompt Loop
     : user enters a command
    -----------------------------------------------------*/
    command_prompt:
    while(1){
        /*---------------------------------------------------------
            Get a new command from the user.

            The user command is stored in a Command struct. If
            the user provides an input which is not a comment
            or blank, then it is parsed to the Command struct
            and processed.
        ----------------------------------------------------------*/
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
        }

        /*-------------------------------------------------------
         Handle built in commands
           cd, exit, and status

           These commands are handled in the main process rather
           than by forking to a child process.
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
            if(WIFEXITED(lastForegroundStatus)){
                printf("Last foreground process, pid %d, exited with status %d\n", 
                        lastForegroundPID, 
                        WEXITSTATUS(lastForegroundStatus));
            } else if (WIFSIGNALED(lastForegroundStatus)){
                printf("The processed received a signal: %d\n", 
                        WTERMSIG(lastForegroundStatus));
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

            switch(childPid){

                // Case -> Fork error
                case -1:
                    perror("fork() failed.");
                    // exit(1);
                    break;

                // Case -> Child Process
                case 0:;

                    /*--------------------------------------------------------------------
                      Signal Handling For Child Processes:
                        1. SIGINT (Child is Background): Ignore (Inherited from Parent)
                        2. SIGINT (Child is Foreground): Default (Set here)
                        3. SIGTSP (All Children): Ignore (Set here)
                    ---------------------------------------------------------------------*/

                    // Ignore SIGTSTP
                    SIGTSTP_action.sa_handler = SIG_IGN;
                    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

                    // Set Default Handling for SIG_INT if this is a foreground process only.
                    if(cmd.background == false){
                        SIGINT_action.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &SIGINT_action, NULL);
                    }


                    /*--------------------------------------------------------------------
                      Prepare arguments for execvp()
                        1. Arguments must be array of pointers
                        2. Last arg must be null pointer
                        3. First arg must be name of command (Example: ls)
                        4. Subsequent args follow the command (Example: -al)
                    ---------------------------------------------------------------------*/
                
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

                    /*-------------------------------------------------
                     Handle Redirects and execute.

                     The symbols for redirect (<, >) have been parsed 
                     out of the args already. The filepaths have also 
                     been parsed out and saved in the cmd struct.

                     Here this information is used to redirect the
                     stdin and stdout streams prior to calling execvp.
                    -------------------------------------------------*/
                    int targetFD;
                    int sourceFD;
                    int result;

                    // Case -> Redirect Input found in command.
                    if(cmd.redir_path_in[0] != 0){
                        // Open source file if found
                        sourceFD = open(cmd.redir_path_in, O_RDONLY);
                        if (sourceFD == -1) { 
                            perror("source open()"); 
                            exit(EXIT_FAILURE); 
	                    }

                        // Redirect the Input stream (0) to the sourceFD
                        result = dup2(sourceFD, 0);
                        if (result == -1){
                            perror("target dup2() - Source");
                            exit(EXIT_FAILURE);
                        }

                    }

                    // Handle Redirect Output if needed
                    if(cmd.redir_path_out[0] != 0){
                        // Create or open output file specified by redirect out path.
                        targetFD = open(cmd.redir_path_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (targetFD == -1) { 
                            perror("target open()"); 
                            exit(EXIT_FAILURE); 
                        } else {
                            printf("File sucessfully made.");
                        }

                        // Redirect stdout (stream 1) to the redirect out file.
                        result = dup2(targetFD, 1);
                        if (result == -1){
                            perror("target dup2() - Destination");
                            exit(EXIT_FAILURE);
                        }
                    }

                    // Execute the command
                    execvp(newargv[0], newargv);
                    perror("Execvp");
                    exit(EXIT_FAILURE);

                /*-----------------------------------------------
                Case -> Default: Parent Process

                The parent handles logic for waiting and 
                monitoring of the child process here. When the
                child process is complete or if the process was
                a non-blocking background process, the parent
                returns control of the program to the user.
                ------------------------------------------------*/
                default:

                    /*--------------------------------------------
                    Case 1: Child Process is Foreground

                    This type of process is called when a user
                    does not use the & symbol in their command.
                    The parent will wait until the process
                    is complete before returning control to the
                    user (blocking).
                    ----------------------------------------------*/
                    if ((cmd.background == 0) | (foreground_only_mode == 1)){
                        lastForegroundPID = childPid;
                        childPid = waitpid(childPid, &wstatus, 0);
                        lastForegroundStatus = wstatus;

                        // Indicate if child was terminated by a signal
                        if (WIFSIGNALED(wstatus)){
                            printf("\nChild process was terminated by signal: %d\n", WTERMSIG(wstatus));
                            lastForegroundStatus = wstatus;
                        }
                    }

                    /*--------------------------------------------
                    Case 2: Child Process is Background

                    This type of process is called when a user
                    includes the & symbol in their command. The
                    child process does not block, but instead
                    runs in the background and control of the
                    program is returned to the user immediately.
                    ----------------------------------------------*/
                    if ((cmd.background == 1) && (foreground_only_mode == 0)){
                        // Announce and add to PID list for tracking.
                        printf("Executing child process %d in the background.\n", childPid);
                        insertPID(PIDS, childPid);
                    }


                    /*--------------------------------------------------------
                     After any new command (blocking or non):
                        1. Check every PID in list using waitpid( WNOHANG )

                        2. If childPid returned is not 0, announce that
                        the process has finished and explain why it exited.
                        This could be termination by reaching end of process
                        or termination by signal.

                        3. Remove any completed process ids from the tracking
                        array.
                    ---------------------------------------------------------*/
                    for(int i = 0; i < MAX_PIDS; i++){
                        if (PIDS[i] != 0){
                            childPid = waitpid(PIDS[i], &wstatus, WNOHANG);

                            // If Child has completed, announce status and remove from PIDS
                            if(childPid != 0){
                                if(WIFEXITED(wstatus)){
                                    printf("Background process, pid %d, exited with status %d\n", childPid, WEXITSTATUS(wstatus));
                                } else if (WIFSIGNALED(wstatus)){
                                    printf("The background process, pid %d, was terminated by signal: %d\n", childPid, WTERMSIG(wstatus));
                                }
                                removePID(PIDS, childPid);
                            }
                        }
                    }

                    // End of Parent Process -> Control returns to the user via command_prompt loop.
                    break;
            }
        }
    }
    
    
}