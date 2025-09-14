#include "systemcalls.h"
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int ret = system(cmd);
    return (ret == 0);
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    va_end(args);
    pid_t pid = fork();
    switch (pid){
    case -1: return false; //Fork failed
    case 0:
        // Child process
        execv(command[0], command);
        // execv only returns on failure
        exit(EXIT_FAILURE);
    default:
        // Parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            return false;
        }
        // Return true if child exited with status 0
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    va_end(args);
    pid_t pid = fork();
    switch (pid){
    case -1: return false; //Fork failed
    case 0:
        // Child process

        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to output file
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);

        // Execute the command
        execv(command[0], command);
        perror("execv"); // only runs if execv fails
        exit(EXIT_FAILURE);
    default:
        // Parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            return false;
        }
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}