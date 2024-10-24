/*******************************************************************************
 *  The BYTE UNIX Benchmarks - Release 3
 *          Module: crud.c   SID: 0.1 10/22/24 17:00:00
 *
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *******************************************************************************
 *  Modification Log:
 *  $Header: crud.c,v 0.1 24/10/22 17:00:00 $
 *
 ******************************************************************************/
/*
 *  crud  -- sit in a tight loop create/read/update/destroy file attempting to bypass caches
 *
 */
char SCCSid[] = "@(#) @(#)crud.c:0.1 -- 10/22/24 17:00:00";

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
int fd;

void cleanup()
{
    close(fd);
    if (access(filename, F_OK) == 0) {
        if (remove(filename) != 0) {
            perror("remove");
        }
    }

}

void report()
{

    cleanup();
	fprintf(stderr,"COUNT|%ld|1|lps\n", iter);
	exit(0);
}


int main(argc, argv)
int	argc;
char	*argv[];
{
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
            cleanup();
            exit(1);
        }
        // write file
        fd = open(filename, O_WRONLY | O_DIRECT | O_SYNC, 0666);
        if ( fd == -1 ) {
            perror("open write only failed:");
            cleanup();
            exit(1);
        }

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("clock_gettime:");
            cleanup();
            exit(1);
        }

        if (posix_memalign((void **)&write_data, ALIGNMENT, ALIGNMENT) != 0) {
            perror("posid_memalign failed:");
            cleanup();
            exit(1);
        }

        snprintf(write_data, ALIGNMENT, "%ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
        write_data[ALIGNMENT - 1] = '\0';
        bytes_written = write(fd, write_data,  ALIGNMENT);
        if (bytes_written == -1) {
            if ( write_data != NULL ) {
                free(write_data);
            }

            perror("write failed:");
            cleanup();
            exit(1);
        }

        if (write_data != NULL) {
            free(write_data);
        }

        if ( close(fd) < 0 ) {
            perror("close failed:");
            cleanup();
            exit(1);
        }

        // read file
        fd = open(filename, O_RDONLY | O_DIRECT | O_SYNC, 0666);
        if ( fd == -1 ) {
            perror("open read only failed:");
            cleanup();
            exit(1);
        }

        if (posix_memalign((void **)&read_data, ALIGNMENT, ALIGNMENT) != 0) {
            perror("posix_memalign failed:");
            cleanup();
            exit(1);
        }

        // Reset file offset to the beginning
		if ( lseek(fd, 0, SEEK_SET) == -1 ) {
            perror("Could not lseek to beginning of file:");
            cleanup();
            exit(1);
        }

        // read file
		ssize_t bytes_read = read(fd, read_data, ALIGNMENT);
		if (bytes_read == -1) {
            if ( read_data != NULL ) {
                free(read_data);
            }
			perror("read file failed:");
            cleanup();
            exit(1);
		}

        if (read_data != NULL ) {
            free(read_data);
        }

        if ( close(fd) < 0 ) {
            perror("close failed:");
            cleanup();
            exit(1);
        }


		// update file
        fd = open(filename, O_WRONLY | O_DIRECT | O_SYNC, 0666);
        if ( fd == -1 ) {
            perror("open for append only failed:");
            cleanup();
            exit(1);
        }

        // seek to end of file
        // sometimes O_APPEND isn't supported with O_DIRECT
        // so we explicitly seek to the end of the file
        if (lseek(fd, 0, SEEK_END) == -1) {
            perror("Could not lseek to end of file:");
            cleanup();
            exit(1);
        }

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("clock_gettime:");
            cleanup();
            exit(1);
        }

        snprintf(write_data, ALIGNMENT, "%ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
        write_data[ALIGNMENT - 1] = '\0';
        bytes_written = write(fd, write_data,  ALIGNMENT);
        if (bytes_written == -1) {
            perror("append write failed:");
            cleanup();
            exit(1);
        }

        if ( close(fd) < 0 ) {
            perror("close failed:");
            cleanup();
            exit(1);
        }

		// delete file
		if (remove(filename) != 0) {
			perror("delete failed");
            cleanup();
            exit(1);
		}

        iter++;
	}

    exit(9);
}

