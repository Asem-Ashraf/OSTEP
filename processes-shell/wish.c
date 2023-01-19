#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#define PRINTABLE_NON_WHITESPACE "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-+=({[,]})"

char error_message[30] = "An error has occurred\n";

// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int count(const char* command){
    int counter=0;
    while(*command != '\0'){
        if (*command == '&') counter++;
        command++;
    }
    return counter;
}

char** argumentv(char* commandUnParsed){
  int counter=0;
  int maxlen=strlen(commandUnParsed);
  char * commandUnParsedCopy= commandUnParsed;
  for (int k = 0 ; k<maxlen; k++) {
      if (isspace(*commandUnParsedCopy)) commandUnParsedCopy++;
      else{
          counter++;
          while (!isspace(*commandUnParsedCopy)) {commandUnParsedCopy++;k++;}
          k--;
      }
  }
  char * * argv=(char**)malloc((counter+1)*8);
  argv[counter]=NULL;
  counter=0;
  commandUnParsedCopy = commandUnParsed;
  for (int k = 0 ; k<maxlen; k++) {
      if (isspace(*commandUnParsedCopy)) commandUnParsedCopy++;
      else{
          argv[counter]=commandUnParsedCopy; // adds the string to the argument vector array.
          while (!isspace(*commandUnParsedCopy)) {commandUnParsedCopy++;k++;}
          *commandUnParsedCopy = '\0'; // Edits the actual string.
          commandUnParsedCopy++ ; // Edits the actual string.
          counter++;
      }
  }
  return argv;
}

void lineHandler(char * command){
    /*char** built_inArgv=argumentv(command);
    if (!strcmp(built_inArgv[0],"cd")) {
        if (strlen((char*)built_inArgv)!=1) {
                write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else {
            chdir(built_inArgv[1]);
        }
    }
    else if (!strcmp(built_inArgv[0],"exit")) {
        if (strlen((char*)built_inArgv)>0) {
                write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else {
        exit(0);
        }
    }
    else if (!strcmp(built_inArgv[0],"path")) {

    }*/



    int Parallel = count(command)+1;
    char* ParallelCommands[Parallel];
    int i =0;
    for (i=0; i<Parallel; i++) {
        ParallelCommands[i]=strsep(&command, "&");
    }
    // fork with the commands and parse it there 
    int pids[Parallel];
    int status[Parallel];
    for (i=0; i<Parallel; i++) {
        pids[i]=fork();
        if (pids[i]==0) // if we are inside the fork
        {
            // check for redirection
            char* commandUnParsed=strsep(&ParallelCommands[i], ">");



            // check for the validity of the redirection
            if (ParallelCommands[i]!=NULL) ParallelCommands[i]= trimwhitespace(ParallelCommands[i]);
            char* rdFile = strsep(&ParallelCommands[i]," \n\t");
            strsep(&ParallelCommands[i],PRINTABLE_NON_WHITESPACE);
            if (ParallelCommands[i]==NULL) {
                // THOSE THREE COMMANDS ARE MADE WITH JUST THE FREOPEN FUNCTION.
                // closeCurrentlyOpenOutputStream(stdin);
                // openThisAsTheOutputStream(ParallelCommands[i]);
                // create the file if it does not exist
                freopen(rdFile, "w", stdout);
            }
            else { // error occured, more than one argument after redirection
                write(STDERR_FILENO, error_message, strlen(error_message));
                /* printf("redirection error\n"); */
                exit(1);
            }

            char** argv=argumentv(commandUnParsed);
            if (-1 == execv(argv[0],argv)) {
                //error message
                // reopen stdout
                write(STDERR_FILENO, error_message, strlen(error_message));
                /* printf("exev\n"); */
                exit(1);
            }
        }
    }
    for (i=0; i<Parallel; i++) {
        waitpid(pids[i],&status[i],0);
    }
}


int main(int argc, char **argv){
    char *command = NULL;
    size_t CommandSize=0;
    if (argc==1) // Interactive mode
    {
        for (;;) {
            // print the prompt and ask for a line of input
            printf("wish> ");
            getline(&command, &CommandSize, stdin);
            lineHandler(command);
            free(command);
        }
    }
    else // Batch mode 
    { 
        // open the file given in the argument and start taking the commands from it
        for (int q;q<argc ; q++) { // For multiple files.
            for (;;) {
                // print the prompt and ask for a line of input
                char *command = NULL;
                size_t CommandSize=0;
                if (-1 == getline(&command, &CommandSize, fopen(argv[q],"r"))){exit(0);}
                lineHandler(command);
                free(command);
            }
        
        }
    }
}

