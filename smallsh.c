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

typedef struct {
    char args[512][2049];
    char redir_path_in[2049];
    char redir_path_out[2049];
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
    temp = malloc(2048);

    // Get process id string to insert into variable expansions
    pidstr = malloc(7);
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
    Function: main ()

************************************************************************/ 

int main(){
    char userInput[2049];
    int cd;

    // Main Prompt
    command_prompt:
    while(1){
        // Reset command struct
        Command cmd = {};
        cmd.background = false;

        // Prompt and get input
        printf(": ");
        fgets(userInput, 2048, stdin);
        
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
        cd = strncmp(cmd.args[0], "cd", 2);
        if(cd == 0){
            changeDir(&cmd);
        }

    }
    
}