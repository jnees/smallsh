#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

typedef struct {
    char args[512][2049];
    char redir_path_in[2049];
    char redir_path_out[2049];
    bool background;
} Command;

/**********************************************************************
    Function: parseCommand(userInput string):

    Parses the user's command line input into a new command struct.
    
    Args:
        input that has been prescreened for comment/null inputs.
    
    Returns:
        command struct

************************************************************************/ 
void parseCommand(char * input, Command *cmd){
    printf("Parsing command: %s", input);

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
                printf(">: %s\n", token);
                strcpy(cmd->redir_path_out, token);
                break;

            // Case token is "<" -> Next token is input redirect filepath
            case '<':
                // Get next token and save to command struct
                token = strtok(NULL, " ");
                printf("<: %s\n", token);
                strcpy(cmd->redir_path_in, token);
                break;

            // case token is "&" -> Process will run in background
            case '&':
                printf("&: %s\n", token);
                cmd->background = true;
                break;

            // case token is all other words -> Append to arguments
            default:    
                printf("default: %s\n", token);
                strcpy(cmd->args[argNumber], token);
                argNumber++;
                break;

        }

        // Next word
        token = strtok(NULL, " "); 
    }
}
    
   

int main(){
    char userInput[2049];

    // Main Prompt
    command_prompt:
    while(1){
        // Reset command struct
        Command cmd = {};
        cmd.background = false;

        printf(": ");
        fgets(userInput, 2048, stdin);

        // Ignore comment
        if ((userInput[0] == '#') | (userInput[0] == '\n')) {
            goto command_prompt;
        } else {
            parseCommand(userInput, &cmd);
            // Debugging
            // printf("arg 0: %s\n", cmd.args[0]);
            // printf("arg 1: %s\n", cmd.args[1]);
            // printf("arg 2: %s\n", cmd.args[2]); 
        }
    }
    
    
}