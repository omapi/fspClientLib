#include <stdio.h>
#include <stdlib.h>
int main()
{
    FILE* fp;
    char buffer[10240];

    fp=popen("./fspClientDemo -id 8b008c8c-2209-97ab-5143-f0a4aa470023 --get Recorder/20160415/200_201_20160415_140858_A00C_cd.wav   -p xxfan123123123","r");
    while(!feof(fp))
    {
        fgets(buffer,sizeof(buffer),fp);
        printf("fgets_%s",buffer);
    }
    pclose(fp);
    return 0;
}