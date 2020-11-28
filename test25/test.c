#include <stdio.h>
#include <string.h>
int main(){
    char string[50];
    char oi[50];
    scanf("%s %s", string, oi);
    int x = strcmp(string, oi);
    printf("%d\n", x);

}