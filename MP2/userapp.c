#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define COMMANDLEN   1000

//check whether pid is in the list (pass admission control)
int read_status(){
    //insert code here...
    return 1;
}

void register_process(unsigned int pid, unsigned int period, unsigned int computation)
{
    char command[COMMANDLEN];
    memset(command, '\0', COMMANDLEN);
    sprintf(command, "echo R,%u,%u,%u > /proc/mp2/status", pid, period, computation);
    system(command);
}

void yield(unsigned int pid)
{
    char command[COMMANDLEN];
    memset(command, '\0', COMMANDLEN);
    sprintf(command, "echo Y,%u > /proc/mp2/status", pid);
    system(command);
    //sleep until the next task arrive
}

void unregister(unsigned int pid)
{
    char command[COMMANDLEN];
    memset(command, '\0', COMMANDLEN);
    sprintf(command, "echo D,%u > /proc/mp2/status", pid);
    system(command);
}

int main(int argc, char* argv[])
{
    int status;
    int expire = atoi(argv[1]);
    time_t start_time = time(NULL);
            
    if (argc == 2) {
        expire = atoi(argv[1]);
    }

    register_process(getpid(), atoi(argv[2]), atoi(argv[3]));
    
    status = read_status();
    if (!status) 
        exit(1);
    
    // break out the while loop if the time is expired
    while (1) {
        if ((int)(time(NULL) - start_time) > expire) {
            break;
        }
    }

    //if finished -> yield()
    unregister(getpid());

    
    return 0;
}
