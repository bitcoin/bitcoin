/*
 * writetest --
 *
 */
#include <sys/types.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(void);

int
main(argc, argv)
    int argc;
    char *argv[];
{
    struct timeval start_time, end_time;
    double usecs;
    long val;
    int bytes, ch, cnt, fd, ops;
    char *fname, buf[100 * 1024];

    bytes = 256;
    fname = "testfile";
    ops = 1000;
    while ((ch = getopt(argc, argv, "b:f:o:")) != EOF)
        switch (ch) {
        case 'b':
            if ((bytes = atoi(optarg)) > sizeof(buf)) {
                fprintf(stderr,
                    "max -b option %d\n", sizeof(buf));
                exit (1);
            }
            break;
        case 'f':
            fname = optarg;
            break;
        case 'o':
            if ((ops = atoi(optarg)) <= 0) {
                fprintf(stderr, "illegal -o option value\n");
                exit (1);
            }
            break;
        case '?':
        default:
            usage();
        }
    argc -= optind;
    argv += optind;

    (void)unlink(fname);
    if ((fd = open(fname, O_RDWR | O_CREAT, 0666)) == -1) {
        perror(fname);
        exit (1);
    }

    memset(buf, 0, bytes);

    printf("Running: %d ops\n", ops);

    (void)gettimeofday(&start_time, NULL);
    for (cnt = 0; cnt < ops; ++cnt) {
        if (write(fd, buf, bytes) != bytes) {
            fprintf(stderr, "write: %s\n", strerror(errno));
            exit (1);
        }
        if (lseek(fd, (off_t)0, SEEK_SET) == -1) {
            fprintf(stderr, "lseek: %s\n", strerror(errno));
            exit (1);
        }
        if (fsync(fd) != 0) {
            fprintf(stderr, "fsync: %s\n", strerror(errno));
            exit (1);
        }
    }
    (void)gettimeofday(&end_time, NULL);

    /*
     * Guarantee end_time fields are greater than or equal to start_time
     * fields.
     */
    if (end_time.tv_sec > start_time.tv_sec) {
        --end_time.tv_sec;
        end_time.tv_usec += 1000000;
    }

    /* Display elapsed time. */
    val = end_time.tv_sec - start_time.tv_sec;
    printf("Elapsed time: %ld:", val / (60 * 60));
    val %= 60 * 60;
    printf("%ld:", val / 60);
    val %= 60;
    printf("%ld.%ld\n", val, end_time.tv_usec - start_time.tv_usec);

    /* Display operations per second. */
    usecs =
        (end_time.tv_sec - start_time.tv_sec) * 1000000 +
        (end_time.tv_usec - start_time.tv_usec);
    printf("%d operations: %7.2f operations per second\n",
        ops, (ops / usecs) * 1000000);

    (void)unlink(fname);
    exit (0);
}

void
usage()
{
    (void)fprintf(stderr,
        "usage: testfile [-b bytes] [-f file] [-o ops]\n");
    exit(1);
}
