#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char const *argv[])
{
    if (argc==1){
        printf("wgrep: searchterm [file ...]\n");
        return 1;
    }
    
    int searchTermSize=strlen(argv[1]);
    FILE* ham;
    char* buffer=NULL;
    size_t lineBufferSize=0;
    int linesize=0;
    //printf("%s",argv[1]);
    if (argc==2)
    {
            while (linesize=getline(&buffer,&lineBufferSize,stdin),linesize!=-1){
                //printf("hi\n");
                if (linesize-searchTermSize<0)
                {
                    goto xxxx;
                }
                
                for (size_t i = 0; i <= linesize-searchTermSize; i++)
                {
                    if (buffer[i]==argv[1][0])
                    {
                        int t=0;
                        while (argv[1][t]==buffer[i+t])
                        {
                            //printf("char: %c , %c\n",argv[1][t],buffer[i+t]);
                            if (argv[1][t+1]=='\0')
                            {
                                printf("%s",buffer);
                                goto xxxx;
                            }
                            t++;
                        }
                    }
                }
                xxxx:
                free(buffer);
                buffer=NULL;
            }
        }
    
    for (size_t i = 2; i < argc; i++)
    {
        ham = fopen(argv[i],"r");
        if (ham == NULL){
            printf("wgrep: cannot open file\n");
            return 1;
        }
        else{
            while (linesize=getline(&buffer,&lineBufferSize,ham),linesize!=-1){
                //printf("hi\n");
                if (linesize-searchTermSize<0)
                {
                    goto xxx;
                }
                
                for (size_t i = 0; i <= linesize-searchTermSize; i++)
                {
                    if (buffer[i]==argv[1][0])
                    {
                        int t=0;
                        while (argv[1][t]==buffer[i+t])
                        {
                            //printf("char: %c , %c\n",argv[1][t],buffer[i+t]);
                            if (argv[1][t+1]=='\0')
                            {
                                printf("%s",buffer);
                                goto xxx;
                            }
                            t++;
                        }
                    }
                }
                xxx:
                free(buffer);
                buffer=NULL;
            }
        }
        fclose(ham);
    }
    return 0;
}
