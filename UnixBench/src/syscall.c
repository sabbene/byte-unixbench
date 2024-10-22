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

#include "timeit.c"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

unsigned long iter;
char filename[20];
int fd;

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

int create_fd()
{
	int fd[2];

	if (pipe(fd) != 0 || close(fd[1]) != 0)
	    exit(1);

	return fd[0];
}

int main(argc, argv)
int	argc;
char	*argv[];
{
        char   *test;
	int	duration;
	int	fd;

	if (argc < 2) {
		fprintf(stderr,"Usage: %s duration [ test ]\n", argv[0]);
        fprintf(stderr,"test is one of:\n");
        fprintf(stderr," m - mix (default)\n c - close\n g - getpid\n e - exec\n f - flock\n o - open\n n - fnctl\n");
		exit(1);
	}
        if (argc > 2)
            test = argv[2];
        else
            test = "mix";

	duration = atoi(argv[1]);

	iter = 0;
	wake_me(duration, report);

        switch (test[0]) {
        case 'm':
            fd = create_fd();
    	    while (1) {
    	    	close(dup(fd));
		        syscall(SYS_getpid);
        		getuid();
	        	umask(022);
    		    iter++;
	        }
    	   /* NOTREACHED */
        case 'c':
           fd = create_fd();
           while (1) {
                close(dup(fd));
                iter++;
           }
           /* NOTREACHED */
        case 'g':
           while (1) {
                syscall(SYS_getpid);
                iter++;
           }
           /* NOTREACHED */
        case 'e':
           while (1) {
                pid_t pid = fork();
                if (pid < 0) {
                    fprintf(stderr,"%s: fork failed\n", argv[0]);
                    exit(1);
                } else if (pid == 0) {
                    execl("/bin/true", "/bin/true", (char *) 0);
                    fprintf(stderr,"%s: exec /bin/true failed\n", argv[0]);
                    exit(1);
                } else {
                    if (waitpid(pid, NULL, 0) < 0) {
                        fprintf(stderr,"%s: waitpid failed\n", argv[0]);
                        exit(1);
                    }
                }
                iter++;
           }
           /* NOTREACHED */
        case 'f':
           	/* Generate a random filename */
            snprintf(filename, sizeof(filename), "file_%d_%ld.tmp", getpid(), (long)time(NULL));


           	/* Open (or create) the file */
           	fd = open(filename, O_CREAT | O_RDWR, 0644);
           	if (fd == -1) {
           		perror("open");
	           	exit(EXIT_FAILURE);
           	}

           	while (1) {
               /* Apply an exclusive lock on the file */
                if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
                    fprintf(stdout,"%s: flock(lock) failed\n", argv[0]);
                    exit(1);
                }

                /* Unlock and close the file to allow new iterations */
                if (flock(fd, LOCK_UN) < 0 ) {
                    fprintf(stdout,"%s: flock(unlock) failed\n", argv[0]);
                    exit(1);
                }

               iter++;
           	}
           /* NOTREACHED */
        case 'o':
            snprintf(filename, sizeof(filename), "file_%d_%ld.tmp", getpid(), (long)time(NULL));

            while (1) {
                int fd = open(filename, O_RDWR | O_CREAT, 0666);
                if (fd == -1) {
                    fprintf(stderr,"%s: open(O_RDWR|O_CREAT) failed\n", argv[0]);
                    exit(1);
                }

                if (close(fd) < 0) {
                    fprintf(stderr,"%s: close failed\n", argv[0]);
                }

                iter++;
            }
           /* NOTREACHED */
        case 'n':
            snprintf(filename, sizeof(filename), "file_%d_%ld.tmp", getpid(), (long)time(NULL));

            while (1) {
                int fd = open(filename, O_RDWR | O_CREAT, 0666);

                // Duplicate the file descriptor using F_DUPFD
                int new_fd = fcntl(fd, F_DUPFD, 0);

                // Set the FD_CLOEXEC flag using F_SETFD
                fcntl(fd, F_SETFD, FD_CLOEXEC);

                // Set the file status flags to non-blocking using F_SETFL
                fcntl(fd, F_SETFL, O_NONBLOCK);

                // File locking using F_SETLK (non-blocking lock)
                struct flock lock;
                memset(&lock, 0, sizeof(lock));
                lock.l_type = F_WRLCK;
                lock.l_whence = SEEK_SET;
                lock.l_start = 0;
                lock.l_len = 0; // Lock the entire file
                fcntl(fd, F_SETLK, &lock);

                // File locking using F_SETLKW (blocking lock)
                lock.l_type = F_RDLCK;
                fcntl(fd, F_SETLKW, &lock);

                // Release the lock
                lock.l_type = F_UNLCK;

                // Clean up
                close(fd);
                close(new_fd);

                iter++;
            }
           /* NOTREACHED */
        }

        exit(9);
}

