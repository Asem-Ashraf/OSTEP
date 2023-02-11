#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void zip(int cross_fileCounter,char prevLetter){
    int rc;
    rc=fwrite(&cross_fileCounter,sizeof(int),1,stdout);
    assert(rc==1);
    rc=fwrite(&prevLetter,sizeof(char),1,stdout);
    assert(rc==1);
}
void printError(){
    printf("wzip: cannot open file\n");
    exit(1);
}
FILE* FopenWrapper(const char* fileName,const char* modes){
    FILE* pFile = fopen(fileName,modes);
    // Error checking.
    if (pFile==NULL) printError();
    return pFile;
}
int main(int argc, char const *argv[]) {
    if (argc == 1 ) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }
    FILE* pFile=NULL;
    char *line=NULL;
    size_t lineSize=0;
    for (int i = 1; i < argc; i++) {
        pFile = FopenWrapper(argv[i],"r");
        char prevLetter;
        int cross_fileCounter;
        int newLineFlag=0;
        while (getline(&line,&lineSize,pFile)!=-1){
            int counter=1;
            if(newLineFlag){
                if (prevLetter==line[0]) counter+=cross_fileCounter; 
                else zip(cross_fileCounter,prevLetter);
                cross_fileCounter=0;
                newLineFlag=0;
            }
            for (size_t k = 0; k < lineSize; k++) {
                if (line[k]==line[k+1]) counter++;
                else {
                    // End of Line reached
                    if (line[k+1]=='\0') {
                        prevLetter=line[k];
                        cross_fileCounter=counter;
                        newLineFlag=1;
                        // If End of File reached
                        if (feof(pFile)!=0) {
                            // If there are more files to be zipped
                            if (i<argc-1) {
                                i++;
                                // Close the current file and open the next file.
                                fclose(pFile);
                                pFile = FopenWrapper(argv[i],"r");
                            }
                            else zip(counter,line[k]);
                        }
                        break;
                    }
                    zip(counter, line[k]);
                    counter = 1;
                }
            }
            free(line);
            line=NULL;
        }
        if (prevLetter=='\n') zip(cross_fileCounter,prevLetter);
        fclose(pFile);
    }
    return 0;
}
