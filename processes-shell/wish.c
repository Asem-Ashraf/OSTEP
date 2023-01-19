#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#define PTR_SIZE 8
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

char** argumentv(char* commandUnParsed,int *counter){
    (*counter)=0;
  int maxlen=strlen(commandUnParsed);
  char * commandUnParsedCopy= commandUnParsed;
  for (int k = 0 ; k<maxlen; k++) {
      if (isspace(*commandUnParsedCopy)) commandUnParsedCopy++;
      else{
          (*counter)++;
          while (!isspace(*commandUnParsedCopy)) {commandUnParsedCopy++;k++;}
          k--;
      }
  }
  char * * argv=(char**)malloc(((*counter+1))*PTR_SIZE);
  /* printf("%d\n",((*counter)+1)*PTR_SIZE); */
  argv[(*counter)]=(void*)0;
  (*counter)=0;
  commandUnParsedCopy = commandUnParsed;
  for (int k = 0 ; k<maxlen; k++) {
      if (isspace(*commandUnParsedCopy)) commandUnParsedCopy++;
      else{
          argv[(*counter)]=commandUnParsedCopy; // adds the string to the argument vector array.
          while (!isspace(*commandUnParsedCopy)) {commandUnParsedCopy++;k++;}
          *commandUnParsedCopy = '\0'; // Edits the actual string.
          commandUnParsedCopy++ ; // Edits the actual string.
          (*counter)++;
      }
  }
  return argv;
}

void lineHandler(char * command){
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
        // check for redirection
        char* commandUnParsed=strsep(&ParallelCommands[i], ">");
        int counter;
        char** argv=argumentv(commandUnParsed,&counter);
        if (!strncmp(argv[0],"cd",2)) {
            /* printf("in cd %d \n",counter); */
            if (counter!=2) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    /* printf("error cd\n"); */
                    continue;
            }
            else {
                if (-1==chdir(argv[1])) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    /* printf("bad direcotry\n"); */
                }
                continue;
            }
        }
        else if (!strncmp(argv[0],"exit",4)) {
            /* printf("in exit\n"); */
            if (counter>1) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    /* printf("error exit\n"); */
                    continue;
            }
            else {
                exit(0);
                continue;
            }
        }
        else if (!strncmp(argv[0],"path",4)) {
            printf("in path\n");
            continue;
        }
        pids[i]=fork();
        if (pids[i]==0) // if we are inside the fork
        {

            if (ParallelCommands[i]!=NULL) { // if there is any redirection
                ParallelCommands[i]= trimwhitespace(ParallelCommands[i]);
                char* rdFile = strsep(&ParallelCommands[i]," >\n\t");
                strsep(&ParallelCommands[i],PRINTABLE_NON_WHITESPACE);
                // check for the validity of the redirection
                if (ParallelCommands[i]==NULL  &&  strlen(rdFile)>1) {
                    // THOSE THREE COMMANDS ARE MADE WITH JUST THE FREOPEN FUNCTION.
                    // closeCurrentlyOpenOutputStream(stdin);
                    // openThisAsTheOutputStream(ParallelCommands[i]);
                    // create the file if it does not exist
                    printf("redirection success.\n");
                    freopen(rdFile, "w", stdout);
                }
                else { // error occured, more than one argument after redirection
                    printf("redirection error\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
            }
            if (-1 == execv(argv[0],argv)) {
                //error message
                write(STDERR_FILENO, error_message, strlen(error_message));
                printf("execv error\n");
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
            if (-1 == getline(&command, &CommandSize,stdin)){exit(0);}
            lineHandler(command);
            free(command);
            command=NULL;
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
                command=NULL;
            }
        
        }
    }
}

