#include <stdio.h>
#define BUFFER_SIZE 255
int main(int argc, char const *argv[])
{
    if (argc==1)
        return 0;

    FILE* ham;
    for (size_t i = 1; i < argc; i++)
    {
        ham = fopen(argv[i],"r");
        if (ham == NULL){
            printf("wcat: cannot open file\n");
            return 1;
        }
        else{
            char buffer[BUFFER_SIZE];
            while (fgets(buffer,BUFFER_SIZE,ham)!=NULL)
                printf("%s",buffer);
            fclose(ham);
        }
    }
    return 0;
}
