#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
    openlog("system_logs", LOG_PID | LOG_CONS, LOG_USER);

    if(argc != 3)
    {
        printf("Printing log error %d\n", argc);
        syslog(LOG_ERR, "2 arguments are not provided.");
        return 1;
    }

    char *file_name = argv[1];
    char *writestr = argv[2];

    FILE *writefile = fopen(file_name, "w");

    fputs(writestr, writefile);
    syslog(LOG_DEBUG , "Writing %s to %s\n", writestr, file_name);

    fclose(writefile);

    return 0;
}