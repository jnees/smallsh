#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

struct command{
    char args[512][2049];
    FILE redirect_in;
    FILE redirect_out;
    bool background;
};

/**********************************************************************
    Function: parseCommand(c)
    
    Args:

************************************************************************/ 
void parseCommand(char * input){
    printf("Parsing command: %s", input);

    
}

int main(){
    // Main Prompt
    char userInput[2049];

    command_prompt:
    while(1){
        printf(": ");
        fgets(userInput, 2048, stdin);

        // Ignore comment
        if ((userInput[0] == '#') | (userInput[0] == '\n')) {
            goto command_prompt;
        } else {
            parseCommand(userInput);
        }
    }
    
    
}