#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
int main(){
    FILE *ptrf;
    ptrf = fopen("inputs/test1.txt","r");
    char line[100];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), ptrf)) {
        printf("%s", line);
    }
    fclose(ptrf);
    return 0;
}