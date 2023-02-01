#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define NULL_CHAR_PADDING 1
#define SLASH_CHAR_PADDING 1
#define PTR_SIZE 8
#define PRINTABLE_NON_WHITESPACE "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.;:,-+*?!@#$%^~`'\"=({[</|\\>]})"

int NumberOfPaths = 1; //  "/bin/"
char** paths=NULL;  
char error_message[30] = "An error has occurred\n";


// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char*
trimwhitespace(char *str){
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



// Funtion to count the number of the parallel commands from the command line string.
// Input:-
//          commands         : The command line string.
// Output:-
//          ParallelCommands : The argument vector.
int
count(const char* commands){
    int ParallelCommands=0;
    while(*commands != '\0'){ // As long as we did not reach the end of the string.
        if (*commands == '&') ParallelCommands++;
        commands++;// Test next character.
    }
    return ParallelCommands;
}



// Funtion to extract argv and argc from a string of characters.
// Input:-
//          commandUnParsed: The unparsed string of character
//                           that contain the command and its
//                           arguments.
//          argc           : A pointer to a variable to save
//                           the number of arguments extracted.
// Output:-
//          argv           : The argument vector.
char** 
argumentv(char* commandUnParsed,int *argc){
    (*argc)=0;
    int maxlen=strlen(commandUnParsed);
    char *commandUnParsedCopy= commandUnParsed;
    for (int k = 0 ; k<maxlen; k++) {
        if (isspace(*commandUnParsedCopy)) commandUnParsedCopy++;
        else{
            (*argc)++;
            while (!isspace(*commandUnParsedCopy)) {commandUnParsedCopy++;k++;}
            k--;
        }
    }
    if ((*argc)==0) return NULL;
    char * * argv=(char**)malloc(((*argc+1))*PTR_SIZE);
    argv[(*argc)]=(void*)0;
    (*argc)=0;
    commandUnParsedCopy = commandUnParsed;
    for (int k = 0 ; k<maxlen; k++) {
        if (isspace(*commandUnParsedCopy)) commandUnParsedCopy++;
        else{
            // adds the string to the argument vector array.
            argv[(*argc)]=commandUnParsedCopy; 
            while (!isspace(*commandUnParsedCopy)) {commandUnParsedCopy++;k++;}
            // Edits the actual string.
            *commandUnParsedCopy = '\0'; 
            // Edits the actual string.
            commandUnParsedCopy++ ; 
            (*argc)++;
        }
    }
    return argv;
}

void 
lineHandler(char* command){
    // Count the number of commands that can run in parallel 
    // by adding 1 to the number of "&" in the input line.
    int Parallel = count(command)+1;
    // Dynamically allocate space for an array of pointers to the commands.
    char** ParallelCommands=(char**)calloc(Parallel,8);
    // Seperate the command from the input line by the "&" sign.
    for (int i=0; i<Parallel; i++) {
        ParallelCommands[i]=strsep(&command, "&");
    }
    // An array to save the pids of the processes to be forked.
    int pids[Parallel];
    // An array to save into the status of the forked processes when they return.
    int status[Parallel];

    for (int i=0; i<Parallel; i++) {
        // Check for redirection
        char* commandUnParsed=strsep(&ParallelCommands[i], ">");

        // Each time a command is going to be parsed, it will use this variable as its argc.
        int argc;

        // A function that returns the argument vector for the input string.
        char** argv=argumentv(commandUnParsed,&argc);

        // If there is not any PRINTABLE_NON_WHITESPACE characters given in the string, 
        // it is an error.
        if (argv==NULL) {
            // If there was a redirection sign without a command.
            if (ParallelCommands[i]!=NULL) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            // If there was not any text at all. It is not an error and it is 
            // treated the same as just pressing enter without typing anything
            // for the shell.(Empty line case)

            // Stop processing this command and continue to the next.
            continue;
        }

        /* * * * * * * * * * * * * * * * 
         * Check for built-in commands.*
         * * * * * * * * * * * * * * * */

        // cd command.
        if (!strncmp(argv[0],"cd",2)) {
            // cd has to have one argument, anything else is an error.
            if (argc!=2) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                // Stop processing this command and continue to the next.
                // This is the behaviour when any command fails.
                continue;
            }
            else {
                if (-1==chdir(argv[1])) {
                    // Bad directory error.
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                // Stop processing this command and continue to the next.
                continue;
            }
        }

            // exit command.
        else if (!strncmp(argv[0],"exit",4)) {
            // exit does not accept any arguments. If arguments are given, 
            // it is an error.
            if (argc>1) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                // Stop processing this command and continue to the next.
                continue;
            }
            else {
                // exit call successful.
                exit(EXIT_SUCCESS);
                continue;
            }
        }
            // path command.
        else if (!strncmp(argv[0],"path",4)) {
            // Everytime path is called the old paths should be cleared and 
            // replaced with the new paths.

            // Freeing old paths.
            for (int f = 0 ; f <NumberOfPaths; f++) {
                free(paths[f]);
            }
            // Freeing paths array.
            free(paths);

            // Updating the number of paths.
            NumberOfPaths=argc-1;

            // Allocating new block of memory to fit the new path count.
            paths=(char**)malloc(NumberOfPaths*PTR_SIZE);

            for (int p = 0;p<NumberOfPaths; p++) {
                // For each path an extra slash "/" has to be added in case that it was not already.
                // This eases the call for execv as it does not care how many "/" are there between directory names.
                // While, if the "/" does not exist between the program name and its containing directory, execv will fail.
                paths[p]=(char*)malloc(strlen(argv[p+1])+NULL_CHAR_PADDING+SLASH_CHAR_PADDING);
                // The paths has to be copies, unlike the commands, because the input line will be freed on each
                // shell prompt. The paths has to be always saved throughout the program.
                strcpy(paths[p],argv[p+1]);
                // Rewrite over the null character with the slash and adds another null character after the slash.
                strcat(paths[p],"/");
            }

            // Another algorithm to add the new paths to the old path rather than re-writing them.
            /*
            paths=realloc(paths, NumberOfPaths+argc-1+1);

            for (int p = NumberOfPaths ;p<NumberOfPaths+argc-1 ; p++) {
                paths[p]=(char*)malloc(strlen(argv[p-NumberOfPaths+1])+NULL_CHAR_PADDING+SLASH_CHAR_PADDING);
                strcpy(paths[p],argv[p-NumberOfPaths+1]);
                strcat(paths[p],"/");
            }
            NumberOfPaths+=argc-1;
            */
            // Stop processing this command and continue to the next.
            continue;
        }

        // If the command is not a built-in command, then fork and execute it.

        /* * * * * * * * * * * * * * *
         * Save the pid of the fork  */
        pids[i]=fork();
        /* * * * * * * * * * * * * * */

        if (pids[i]==0)
        {
            // if there is any redirection with the command.
            if (ParallelCommands[i]!=NULL) { 
                // trim the whitespaces before and after the redirection file.
                ParallelCommands[i]= trimwhitespace(ParallelCommands[i]);
                // Save the string of the redirection file.
                char* rdFile = strsep(&ParallelCommands[i]," >\n\t");
                // Then, check for any other characters after the name of the redirection file.
                strsep(&ParallelCommands[i],PRINTABLE_NON_WHITESPACE);

                // If there is not any more PRINTABLE_NON_WHITESPACE characters, 
                // then execute the redirection.
                if (ParallelCommands[i]==NULL  &&  strlen(rdFile)>1) {
                    // THOSE THREE PSEUDO-COMMANDS ARE MADE WITH JUST THE FREOPEN FUNCTION.
                    // closeOutputStream(stdin);
                    // openAsTheOutputStream(ParallelCommands[i]);
                    // create the file if it does not exist
                    freopen(rdFile, "w", stdout);
                }
                else { // error occured, more than one argument after redirection sign.
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(EXIT_FAILURE);
                }
            }
            // Execute the command as if it is in the current directory.
            if (-1 == execv(argv[0],argv)) {
                // If the above failed.
                // Check for the commands in the directories provided in paths.
                for (int p = 0;p<NumberOfPaths; p++) {
                    // Append the first argv element to each directory  in paths then execute.
                    int pathsize = strlen(paths[p])  +  strlen(argv[0]-1);
                    char correctPath[pathsize];
                    strcpy(correctPath, paths[p]);
                    strcpy(correctPath+strlen(paths[p]), argv[0]);

                    execv(correctPath,argv);
                }
                // If the program is not in any of the directories then the command does not exit.
                write(STDERR_FILENO, error_message, strlen(error_message));
                // Exit the fork and return to the shell process.
                exit(EXIT_FAILURE);
            }
        }
    }
    // Wait for all the process to finish execution before prompting for the next line.
    for (int i=0; i<Parallel; i++) {
        waitpid(pids[i],&status[i],0);
    }
}


int 
main(int argc, char **argv){
    // Initializing the global variable that contain the initial path of programs.
    // the variable contents has to be dynamically allocated,
    // because they need to be freed on the next call of path.
    paths=(char**)malloc(PTR_SIZE);
    char* defaultLocation=(char*)malloc(strlen("/bin/")+1);
    strcpy(defaultLocation,"/bin/");
    paths[0]=defaultLocation;

    char *command = NULL;
    size_t CommandSize=0;

    if (argc==1) // Interactive mode
        {
            for (;;) {
                // print the prompt and ask for a line of input
                printf("wish> ");
                // if getline ever fails the shell should exit with the common error message.
                if (-1 == getline(&command, &CommandSize,stdin)){
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(EXIT_FAILURE);
                }
                // Handle the command.
                lineHandler(command);
                // free the space of the executed command.
                free(command);
                // set the command to NULL so that getline() allocates new block of memory.
                command=NULL;
            }
        }
    else // Batch mode 
    { 
        if (argc>2) {  
            // if the shell is called with more than one file argument,
            // exit with error.
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(EXIT_FAILURE);
        }
        // open the file given in the argument and start taking the commands from it
        FILE *script = fopen(argv[1],"r");
        // Failing to open the file is an error.
        if (script==NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(EXIT_FAILURE);
        }
        for (;;) {
            if (-1 == getline(&command, &CommandSize,script)){exit(0);}
            // Handle the command.
            lineHandler(command);
            // free the space of the executed command.
            free(command);
            // set the command to NULL so that getline() allocates new block of memory.
            command=NULL;
        }
    }
}

