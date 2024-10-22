/*******************************************************************************
 *  The BYTE UNIX Benchmarks - Release 3
 *          Module: syscall.c   SID: 3.3 5/15/91 19:30:21
 *
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Ben Smith, Rick Grehan or Tom Yager at BYTE Magazine
 *	ben@bytepb.byte.com   rick_g@bytepb.byte.com   tyager@bytepb.byte.com
 *
 *******************************************************************************
 *  Modification Log:
 *  $Header: syscall.c,v 3.4 87/06/22 14:32:54 kjmcdonell Beta $
 *  August 29, 1990 - Modified timing routines
 *  October 22, 1997 - code cleanup to remove ANSI C compiler warnings
 *                     Andy Kahn <kahn@zk3.dec.com>
 *
 ******************************************************************************/
/*
 *  syscall  -- sit in a loop calling the system
 *
 */
char SCCSid[] = "@(#) @(#)syscall.c:3.3 -- 5/15/91 19:30:21";

#define _GNU_SOURCE

#include "timeit.c"
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define ALIGNMENT 4096

unsigned long iter;
char filename[20];

void report()
{
    if (access(filename, F_OK) == 0) {
        if (remove(filename) != 0) {
            perror("remove");
        }
    }

	fprintf(stderr,"COUNT|%ld|1|lps\n", iter);
	exit(0);
}

int main(argc, argv)
int	argc;
char	*argv[];
{
    int fd;
	int	duration;
    char *write_data;
    char *read_data;
    int bytes_written;
    struct timespec ts;

	duration = atoi(argv[1]);

	iter = 0;
	wake_me(duration, report);

    snprintf(filename, sizeof(filename), "file_%d_%ld.tmp", getpid(), (long)time(NULL));

    while (1) {

        // create file
        fd = open(filename, O_RDWR | O_CREAT | O_DIRECT | O_SYNC | O_EXCL, 0666);
        if ( fd == -1 ) {
            perror("create failed:");
            exit(1);
        }
        if ( close(fd) < 0 ) {
            perror("close failed:");
            exit(1);
        }
        // write file
        fd = open(filename, O_WRONLY | O_DIRECT | O_SYNC, 0666);
        if ( fd == -1 ) {
            perror("open write only failed:");
            exit(1);
        }

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("clock_gettime:");
            exit(1);
        }

        if (posix_memalign((void **)&write_data, ALIGNMENT, ALIGNMENT) != 0) {
            close(fd);
            perror("posid_memalign failed:");
            exit(1);
        }

        snprintf(write_data, ALIGNMENT, "%ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
        write_data[ALIGNMENT - 1] = '\0';
        bytes_written = write(fd, write_data,  ALIGNMENT);
        if (bytes_written == -1) {
            close(fd);
            free(write_data);
            perror("write failed:");
            exit(1);
        }

        free(write_data);

        if ( close(fd) < 0 ) {
            perror("close failed:");
            exit(1);
        }

        // read file
        fd = open(filename, O_RDONLY | O_DIRECT | O_SYNC, 0666);
        if ( fd == -1 ) {
            perror("open read only failed:");
            exit(1);
        }

        if (posix_memalign((void **)&read_data, ALIGNMENT, ALIGNMENT) != 0) {
            close(fd);
            perror("posid_memalign failed:");
            exit(1);
        }

		lseek(fd, 0, SEEK_SET); // Reset file offset to the beginning
		ssize_t bytes_read = read(fd, read_data, ALIGNMENT);
		if (bytes_read == -1) {
            free(read_data);
			close(fd);
			perror("read file failed:");
			exit(1);
		}

        free(read_data);

        if ( close(fd) < 0 ) {
            perror("close failed:");
            exit(1);
        }

		// update file
        fd = open(filename, O_WRONLY | O_APPEND | O_DIRECT | O_SYNC, 0666);
        if ( fd == -1 ) {
            perror("open  append only failed:");
            exit(1);
        }

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("clock_gettime:");
            exit(1);
        }

        snprintf(write_data, ALIGNMENT, "%ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
        write_data[ALIGNMENT - 1] = '\0';
        bytes_written = write(fd, write_data,  ALIGNMENT);
        if (bytes_written == -1) {
            close(fd);
            perror("append write failed:");
            exit(1);
        }

        if ( close(fd) < 0 ) {
            perror("close failed:");
            exit(1);
        }

		// delete file
		if (remove(filename) != 0) {
			perror("delete failed");
			exit(1);
		}

        iter++;
	}

    exit(9);
}

