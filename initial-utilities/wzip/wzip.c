#include <stdio.h>
#include <stdlib.h>
int main(int argc, char const *argv[])
{
    if (argc ==1 )
    {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }
    FILE* ham=NULL;
    char *buffer=NULL;
    size_t linebufsize=0;
    int linesize=0;
    
    for (size_t i = 1; i < argc; i++)
    {
        ham = fopen(argv[i],"r");
        if (ham==NULL)
        {
            printf("wzip: cannot open file\n");
            exit(1);
        }
        char theLetter;
        int county;
        int flag=0;
        while (linesize=getline(&buffer,&linebufsize,ham),linesize!=-1){
            int counter=1;
            if(flag){
                if (theLetter==buffer[0])
                {
                    counter+=county;county=0;
                }
                else{
                    fwrite(&county,sizeof(int),1,stdout);
                    fwrite(&theLetter,sizeof(char),1,stdout);
                    county = 0;
                }
                flag=0;
            }
            for (size_t k = 0; k < linesize; k++)
            {
                if (buffer[k]==buffer[k+1])
                {
                    counter++;
                }
                else
                {
                    if (buffer[k+1]=='\0')
                    {
                        if (linesize==1)
                        {
                            
                        }
                        
                        //printf("hi");
                        theLetter=buffer[k];
                        county=counter;
                        if (feof(ham)!=0)
                        {
                            if (i<argc-1)
                            {
                                i++;
                                fclose(ham);
                                ham = fopen(argv[i],"r");
                                if (ham==NULL)
                                {
                                    printf("wzip: cannot open file\n");
                                    exit(1);
                                }
                                flag=1;
                            }
                            else{
                                fwrite(&counter,sizeof(int),1,stdout);
                                fwrite(&buffer[k],sizeof(char),1,stdout);
                            }
                        }
                        else flag=1;
                        break;
                    }
                    fwrite(&counter,sizeof(int),1,stdout);
                    fwrite(&buffer[k],sizeof(char),1,stdout);
                    counter = 1;
                }
            }
            free(buffer); buffer=NULL;
        }
        if (theLetter=='\n')
        {
                                fwrite(&county,sizeof(int),1,stdout);
                                fwrite(&theLetter,sizeof(char),1,stdout);
        }
        
        fclose(ham);
    }
    return 0;
}
