
/* A string tokenization library.

   Author: RR
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define END_OF_STRING -1


/* Handler in the event of a heap allocation error.

   Parameters: N/A
   Returns: N/A
*/ 
void handleHeapError() {
  perror("Error --- out of heap space. Exiting...\n");
  exit(-1);
}


/* Returns the number of tokens in the supplied string.

   Parameters:
       cmdLine - the string to be tokenized

   Returns:
       The number of white-space separated tokens that were found in
       cmdLine.
*/
int countTokens(const char* cmdLine) {
  // handle the empty command
  if (strlen(cmdLine) == 0)
    return 0;

  // initialize numTokens depending on whether the command begins
  // with a white space.
  int numTokens;
  if (isspace(cmdLine[0]))
    numTokens = 0;
  else
    numTokens = 1;

  int i = 1;
  while (cmdLine[i] != '\0') {
    // is this a token boundary?
    if ((!isspace(cmdLine[i])) && (isspace(cmdLine[i-1])))
      numTokens++;
    i++;
  }

  return numTokens;
}


/* Returns the next token that can be read parsed from the supplied string.

   Parameters:
       cmdLine - the string to be tokenized.
       start - the index position in cmdLine from which to start scanning
               for the next token.

   Returns:
       The ending index of the next token. Returns an END_OF_STRING special
       value in case no more tokens are left to read. Further, the value
       pointed to by the input parameter start may be modified in case some
       white space characters need to be consumed, before the start of the
       next token.
*/ 
int getNextToken(const char* cmdLine, int* start) {
  int i = *start;
  
  // seek to first non-space character  
  while ((cmdLine[i] != '\0') && (isspace(cmdLine[i]))) {
    i++;
  }

  if (cmdLine[i] == '\0') // no more tokens
    return END_OF_STRING;
  
  // this is the new start; now go until we find the end of this token
  *start = i;
  while ((cmdLine[i] != '\0') && (!isspace(cmdLine[i])))
    i++;
  
  return i;
}


/* Returns an array of strings containing tokens extracted from the supplied 
   string.

   Parameters:
       cmdLine - the string to be tokenized
       background - a pointer to a value that is set depending on whether the
                    supplied string describes a command to be executed in
		    "background" mode, i.e., whether the last non-white space
		    character in the command string is an &. If it is an &,
		    then the command is to be run in background mode, and
		    *background is set to 1; otherwise, it is set to 0.

   Returns:
       A NULL-terminated array of char*s, where each char* points to a
       string containing an extracted token.
*/
char** parseCommand(const char* cmdLine, int* background) {
  int numTokens = countTokens(cmdLine);

  // assume foreground mode by default
  *background = 0;
 
  // +1 for the NULL terminator in the array;
  char** args = (char**)malloc(sizeof(char*) * (numTokens + 1));

  if (!args)
    handleHeapError();
  
  // extract tokens one after the other and fill up the args array
  int start = 0;
  int end;
  int tokenLength;
  for (int i = 0; i < numTokens; i++) {
    // find the next token and determine its length
    end = getNextToken(cmdLine, &start);
    tokenLength = end - start;
    args[i] = (char*)malloc(sizeof(char) * (tokenLength + 1)); // +1 for NULL

    if (!args[i])
      handleHeapError();

    // copy the token contents into the args array
    strncpy(args[i], (cmdLine + start), tokenLength);
    args[i][tokenLength] = '\0';
    start = end;
  }

  // Case 1: background mode where '&' is its own token
  if ((numTokens > 0) && (strcmp(args[numTokens-1], "&") == 0)) {
    *background = 1;
    args = realloc(args, sizeof(char*)*numTokens);
    numTokens--;
  }
  // Case 2: background mode where & is part of last token
  else if (numTokens > 0) { 
    int length = strlen(args[numTokens-1]);
    if (args[numTokens-1][length-1] == '&') { // realloc last string alone
      *background = 1;
      args[numTokens-1] = realloc(args[numTokens-1], sizeof(char)*length);
      args[numTokens-1][length-1] = '\0';
    }
  }

  args[numTokens] = NULL; // NULL terminate args array
    
  return args;
}


