/* 
 * Author: Ravi Agarwal
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>

/* 
 *@brief function to write a string to a file
 *       using FILE I/O in linux POSIX systems
 *
 *@param argv[1] file path to be written
 *@param argv[2] string to be written
 *
 */ 
int main(int argc, char *argv[]){
	// check argumnet count 3 because 1st argument is the file name
	if(argc != 3){
		return 1;
	}

	const char *writefile = argv[1];
	const char *writestr = argv[2];

	// open syslog with LOG_USER facility
	openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

	syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

	// open the file in write only mode, with rwx-rwx-rwx- permissions
	int fd = open(writefile, O_CREAT|O_RDWR, 0777);
	if (fd == -1){
		syslog(LOG_ERR, "Error in opening the file %s: %s", writefile, strerror(errno));
		closelog();
	}

	// writing string to the file
	ssize_t bytes_written = write(fd, writestr, strlen(writestr));
	if (bytes_written == -1){
		syslog(LOG_ERR, "Error writing to the file %s: %s", writefile, strerror(errno));
		close(fd);
		closelog();
	}

	// close the file
	if (close(fd) == -1){
		syslog(LOG_ERR, "Error closing the file %s: %s", writefile, strerror(errno));
		closelog();
	}

	closelog();

	return 0; 


}










