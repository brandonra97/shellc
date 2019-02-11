/* A c implementation of a shell terminal with built in commands.

   Author: Brandon S Ra

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "parser.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define maxLen 1000 //maximum character length of command (without null)
#define ringSize 10 //number of commands that will be updated in history

int ringStart = -1; //index of ringBuffer to the newest element
int ringEnd = 0; //index of ringBuffer to the oldest element
int ringCount = 0; //counts how many elements are in the ringBuffer
int cmdID = 1; //the commandID for commands

typedef struct{ //a struct for storing history of commands
  char command[maxLen];
  unsigned int commandID;
} history_t;

//circular buffer for storing history of commands
history_t ringBuffer[sizeof(history_t) * ringSize];

/* Executes a command in foreground.

   Parameters:
      cmdParsed - an array of strings with the arguments of the command line
      cmdLine - the command inputted

   Returns:
      None.
*/
void foreground(char** cmdParsed, char* cmdLine){
  pid_t pid = fork();
  if(pid < 0){
    printf("error in forking\n");
    fflush(stdout);
    exit(-1);
  }
  else if(pid == 0){
    if(execvp(cmdParsed[0], cmdParsed) < 0){
      printf("%s: command not found\n", cmdLine);
      fflush(stdout);
      exit(0);
    }
  }
  else{
    waitpid(pid, NULL, 0);
  }
}

/* A signal handler for background processes.

   Parameters:
      sigNum- an integer value that represent the type of signal

   Returns:
      None.
*/
void handler(int sigNum){
  waitpid(-1, NULL, 0);
}

/* Executes a command in background.

   Parameters:
      cmdParsed - an array of strings with the arguments of the command line
      cmdLine - the command inputted

   Returns:
      None.
*/
void background(char** cmdParsed, char* cmdLine){
  signal(SIGCHLD, handler);
  pid_t pid = fork();
  if(pid < 0){
    printf("error in forking\n");
    fflush(stdout);
    exit(-1);
  }
  else if(pid == 0){
    if(execvp(cmdParsed[0], cmdParsed) < 0){
      printf("%s: command not found\n", cmdLine);
      fflush(stdout);
      exit(0);
    }
  }
}

/* An indexer for the ringBuffer indices. Appropriately updates the index.

   Parameters:
      index - an integer pointer to the index value that needs to be updated.

   Returns:
      None.
*/
void indexer(int* index){
  int temp = *index + 1;
  if(temp >= ringSize){
    *index = 0;
  }
  else{
    *index = temp;
  }
}

/* Records the command into history by storing them into the ringBuffer.

   Parameters:
      cmdLine - the command inputted to be stored

   Returns:
      None.
*/
void recHistory(char* cmdLine){
  if(ringCount < ringSize){
    indexer(&ringStart);
    strcpy((*(ringBuffer + ringStart)).command, cmdLine);
    (*(ringBuffer + ringStart)).commandID = cmdID;
    cmdID++;
    if(ringCount == ringSize){
      ringCount++;
    }
    ringCount++;
  }
  else{
    indexer(&ringStart);
    indexer(&ringEnd);
    strcpy((*(ringBuffer + ringStart)).command, cmdLine);
    (*(ringBuffer + ringStart)).commandID = cmdID;
    cmdID++;
  }
}

/* Prints the current list of history of commands. Also records the history
command as well.

   Parameters:
      cmdLine - the command inputted

   Returns:
      None.
*/
void history(char* cmdLine){
  recHistory(cmdLine);
  int index = ringEnd;
  for(int i = 0; i < ringCount; i++){
    printf("       %d %s\n", (*(ringBuffer + index)).commandID,
        (*(ringBuffer + index)).command);
    fflush(stdout);
    indexer(&index);
  }
}

/* A function that determines which function to call for the inputted command.
It appropriately directs commands to foreground or background based on the
background integer value. It also directs the program to the history function
if the command is history.

   Parameters:
      cmdParsed - an array of strings with the arguments of the command line
      cmdLine - the command inputted
      bg - an integer value that determines whether a command will be run
        in background or foreground. 0 if foreground and 1 if background.

   Returns:
      None.
*/
void programCall(char** cmdParsed, char* cmdLine, int bg){
  if(!(strcmp(cmdLine, "history"))){
    history(cmdLine);
  }
  else{
    if(bg == 0){
      foreground(cmdParsed, cmdLine);
      recHistory(cmdLine);
    }
    else{
      background(cmdParsed, cmdLine);
      recHistory(cmdLine);
    }
  }
}

/* Executes a command based on the commandID given if the command is still
stored in history.

   Parameters:
      cmdID - the commandID of the command in history you wish to re-execute.
      cmdLine - the command inputted

   Returns:
      None.
*/
void callHist(int cmdID, char* cmdLine){
  int index = ringEnd;
  for(int i = 0; i < ringCount; i++){
    if((*(ringBuffer + index)).commandID == cmdID){
      int bg = 0;
      char **cmdParsed = parseCommand((*(ringBuffer + index)).command,
        &bg);
      programCall(cmdParsed, (*(ringBuffer + index)).command, bg);
      return;
    }
    indexer(&index);
  }
  printf("%s: event not found\n", cmdLine);
  fflush(stdout);
}

/* A prologue for the shell which prints the prompt and accepts input command
from the user. Also removes the newline character once the command is inputted.

   Parameters:
      cmdLine - the string variable to store the command inputted into

   Returns:
      None.
*/
void cmdPrologue(char* cmdLine){
  printf("rashell> ");
  fflush(stdout);
  fgets(cmdLine, maxLen + 1, stdin);
  strtok(cmdLine, "\n");
}

/* A wrapper function for the shell implementation that reads in the commands
from the user and executes commands as needed.

   Parameters:
      None.

   Returns:
      None.
*/
void readCommand(){
  char cmdLine[maxLen + 1];
  int bg = 0;
  cmdPrologue(cmdLine);
  while(strcmp(cmdLine, "exit")){
    if(!(strcmp(cmdLine, "history"))){
      history(cmdLine);
    }
    else{
      char **cmdParsed = parseCommand(cmdLine, &bg);
      if(*(cmdParsed) == NULL){
        cmdPrologue(cmdLine);
        continue;
      }
      else if(*(*cmdParsed) == '!'){
        char temp[maxLen];
        strcpy(temp, *(cmdParsed));
        int cmdID = atoi(temp + 1);
        callHist(cmdID, cmdLine);
        cmdPrologue(cmdLine);
        continue;
      }
      programCall(cmdParsed, cmdLine, bg);
      free(cmdParsed);
    }
    cmdPrologue(cmdLine);
  }
}

int main(){
  readCommand();
  return 0;
}
