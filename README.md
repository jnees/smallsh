# Smallsh
CS344 Operating Systems, Portfolio Assignment: Smallsh  
Oregon State University  
  
## Compiling instructions:

#### Run the following command to compile:  
$ gcc --std=c99 -o smallsh smallsh.c  

#### Alternately, use the Makefile to execute this same command:
$ make
  
#### This creates an executable file, movies, which can be run directly. Requires no args.
$ ./smallsh

## Testing:

#### This assignment includes a test file for grading. To run this file after compiling:

$ make test

#### Or alternately, chmod the file and run after compiling. Test file must be in same dir as smallsh executable.
$ gcc --std=c99 -o smallsh smallsh.c  
$ chmod +x ./p3testscript
$ ./p3testscript > mytestresults 2>&1 