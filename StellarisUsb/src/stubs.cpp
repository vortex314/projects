/*
 * stubs.cpp
 *
 *  Created on: 26-aug.-2014
 *      Author: lieven2
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>



#include "errno.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int errno;

size_t strlen(const char *s){
	int count =0;
	while(*s) {
		count++;
		s++;
	}
	return count;
}

void _exit(int code){
while(1);
}

void __aeabi_atexit(){
	return;
}

void __exidx_end(){
while(1);
}

void __exidx_start(){
while(1);
}
/*
  kill
  Send a signal. Minimal implementation:
  */
 int _kill(int pid, int sig) {
 	errno = EINVAL;
 	return (-1);
 }

 //void abort(){};

 /*
  link
  Establish a new name for an existing file. Minimal implementation:
  */





 /*
  getpid
  Process-ID; this is sometimes used to generate strings unlikely to conflict with other processes. Minimal implementation, for a system without processes:
  */

 int _getpid() {
 	return 1;
 }

int _ctype(char c) {
	return 0;
}
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef STDOUT_USART
#define STDOUT_USART 2
#endif

#ifndef STDERR_USART
#define STDERR_USART 2
#endif

#ifndef STDIN_USART
#define STDIN_USART 2
#endif
 /*
  isatty
  Query whether output stream is a terminal. For consistency with the other minimal implementations,
  */
 int _isatty(int file) {
 	switch (file) {
 		case STDOUT_FILENO:
 		case STDERR_FILENO:
 		case STDIN_FILENO:
 		return 1;
 		default:
 		//errno = ENOTTY;
 		errno = EBADF;
 		return 0;
 	}
 }
 /*
  write
  Write a character to a file. `libc' subroutines will use this system routine for output to all files, including stdout
  Returns -1 on error or number of bytes sent
  */
 int _write(int file, char *ptr, int len) {
  return 0;
  }
 /*
  read
  Read a character to a file. `libc' subroutines will use this system routine for input from all files, including stdin
  Returns -1 on error or blocks until the number of characters have been read.
  */

 int _read(int file, void *ptr, size_t len) {

 	return 0;
 }
/*
 int stat(const char *filepath, struct stat *st) {
 	st->st_mode = S_IFCHR;
 	return 0;
 }
*/
 /*
   lseek
   Set position in a file. Minimal implementation:
   */
  off_t lseek(int file, off_t ptr, int dir) {
  	return 0;
  }

 int _close(int file) {
  	return -1;
  }
 /*
  fstat
  Status of an open file. For consistency with other minimal implementations in these examples,
  all files are regarded as character special devices.
  The `sys/stat.h' header file required is distributed in the `include' subdirectory for this C library.
  */

 int _fstat(int file, struct stat *st) {
 	st->st_mode = S_IFCHR;
 	return 0;
 }

 /*
  link
  Establish a new name for an existing file. Minimal implementation:
  */

 int _link(char *old, char *nw) {
 	errno = EMLINK;
 	return -1;
 }

 /*
  lseek
  Set position in a file. Minimal implementation:
  */
 int _lseek(int file, int ptr, int dir) {
 	return 0;
 }

#ifdef __cplusplus
};
#endif
