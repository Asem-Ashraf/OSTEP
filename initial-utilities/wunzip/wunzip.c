#include <stdio.h>
#include <stdlib.h>
int main(int argc, char const *argv[])
{
    if (argc ==1)
    {
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }
    FILE* ham=NULL;
    
    for (size_t i = 1; i < argc; i++)
    {
        //printf("%d, %d\n",(int)i,argc);
        ham = fopen(argv[i],"r");
        if (ham==NULL)
        {
            printf("wzip: cannot open file\n");
            exit(1);
        }
        int count=0;
        char letter;
        while (fread(&count,sizeof(int),1,ham),fread(&letter,sizeof(char),1,ham),feof(ham)==0)
        {
            //printf("%d\n",feof(ham));
            //printf("%d\n",count);
            //printf("%c\n",letter);
            for (size_t i = 0; i < count; i++)
            {
                printf("%c",letter);
            }
            //printf("%d\n",feof(ham));
        }
        fclose(ham);
    }
    return 0;
}
