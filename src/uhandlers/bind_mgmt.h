#ifndef BIND_MGMT_H
#define BIND_MGMT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
* Handles the unbinding of device drivers from input devices
*/
int unbind_handler()
{
    pid_t child_pid;
    int wstatus;

    child_pid = fork();
    if (child_pid == -1)
    {
        perror("unetlink: fork");
        return -1;
    }

    if (child_pid == 0)      /* This is the child process */
    {
        /* call the unbind script */
        char script_path[] = "/home/pi/scripts/pibox/unbind_input_drivers.sh";
        if (execl(script_path, script_path, NULL) < 0)
        {
            perror("key_packet: execl");
            return -1;
        }
    }
    else            /* Parent must wait for this process to finish */
    {
        while(waitpid(child_pid, &wstatus, 0) > 0);
    }
    return 0;
}

#endif  // BIND_MGMT_H