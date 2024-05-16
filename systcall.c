#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>

#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE * KILOBYTE)

void printUsage() {
    printf("Usage: ./systcall <filename>\n");
}

double measureReadTime(const char* filename, int block_size, int block_count) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    char buffer[block_size];
    ssize_t bytesRead;

    clock_t start, end;
    double totalTime = 0;

    for (int i = 0; i < block_count; ++i) {
        start = clock();

        bytesRead = read(fd, buffer, block_size);

        end = clock();
        double elapsedTime = ((double)(end - start)) / CLOCKS_PER_SEC;

        if (bytesRead == -1) {
            perror("Error reading from file");
            exit(EXIT_FAILURE);
        }

        totalTime += elapsedTime;
    }

    close(fd);

    return totalTime;
}

void printFileSize(int block_size, int block_count) {
    double fileSizeKB = (double)block_size * block_count / KILOBYTE;
    double fileSizeMB = fileSizeKB / KILOBYTE;

    printf("Block Size : %d Bytes\n", block_size);
}

void printPerformance(const char* filename, int block_size, int block_count) {
    double totalTime = measureReadTime(filename, block_size, block_count);

    // Calculate performance in MiB/s and B/s
    double totalDataSizeMB = (double)block_size * block_count / MEGABYTE;
    double performanceMBs = totalDataSizeMB / totalTime;
    double performanceBs = (double)block_count / totalTime;

    printf("Time taken to read: %.6f seconds\n", totalTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
}

void measureSystemCallPerformance(const char* filename) {
    printf("\nSystem Call Performance Measurement:\n");
    printf("\n");

    // File descriptor for the file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for system call measurement");
        exit(EXIT_FAILURE);
    }

    // Measure write system call performance
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Writing a small buffer to the file
    char buffer[512];
    write(fd, buffer, sizeof(buffer));

    gettimeofday(&end, NULL);

    double elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // Calculate performance in MiB/s and B/s
    double totalDataSizeMB = (double)512 / MEGABYTE; // Assuming a small buffer of 512 bytes for system calls
    double performanceMBs = totalDataSizeMB / elapsedTime;
    double performanceBs = 512 / elapsedTime;

    printf("System Call: write\n");
    printf("Time taken: %.6f seconds\n", elapsedTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
    printf("\n");

    // Perform fdatasync system call and measure its performance
    gettimeofday(&start, NULL);
    fdatasync(fd);
    gettimeofday(&end, NULL);

    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // Calculate performance in MiB/s and B/s
    totalDataSizeMB = (double)512 / MEGABYTE; // Assuming a small buffer of 512 bytes for system calls
    performanceMBs = totalDataSizeMB / elapsedTime;
    performanceBs = 512 / elapsedTime;

    printf("System Call: fdatasync\n");
    printf("Time taken: %.6f seconds\n", elapsedTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
    printf("\n");

    // Perform fsync system call and measure its performance
    gettimeofday(&start, NULL);
    fsync(fd);
    gettimeofday(&end, NULL);

    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // Calculate performance in MiB/s and B/s
    totalDataSizeMB = (double)512 / MEGABYTE; // Assuming a small buffer of 512 bytes for system calls
    performanceMBs = totalDataSizeMB / elapsedTime;
    performanceBs = 512 / elapsedTime;

    printf("System Call: fsync\n");
    printf("Time taken: %.6f seconds\n", elapsedTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
    printf("\n");

    // Perform fstat system call and measure its performance
    struct stat fileStat;
    gettimeofday(&start, NULL);
    fstat(fd, &fileStat);
    gettimeofday(&end, NULL);

    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // Calculate performance in MiB/s and B/s
    totalDataSizeMB = (double)512 / MEGABYTE; // Assuming a small buffer of 512 bytes for system calls
    performanceMBs = totalDataSizeMB / elapsedTime;
    performanceBs = 512 / elapsedTime;

    printf("System Call: fstat\n");
    printf("Time taken: %.6f seconds\n", elapsedTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
    printf("\n");

    // Perform lseek system call and measure its performance
    gettimeofday(&start, NULL);
    lseek(fd, 0, SEEK_SET);
    gettimeofday(&end, NULL);

    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // Calculate performance in MiB/s and B/s
    totalDataSizeMB = (double)512 / MEGABYTE; // Assuming a small buffer of 512 bytes for system calls
    performanceMBs = totalDataSizeMB / elapsedTime;
    performanceBs = 512 / elapsedTime;

    printf("System Call: lseek\n");
    printf("Time taken: %.6f seconds\n", elapsedTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
    printf("\n");

    // Perform getpid system call and measure its performance
    gettimeofday(&start, NULL);
    getpid();
    gettimeofday(&end, NULL);

    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // Calculate performance in MiB/s and B/s
    totalDataSizeMB = (double)512 / MEGABYTE; // Assuming a small buffer of 512 bytes for system calls
    performanceMBs = totalDataSizeMB / elapsedTime;
    performanceBs = 512 / elapsedTime;

    printf("System Call: getpid\n");
    printf("Time taken: %.6f seconds\n", elapsedTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
    printf("\n");

    // Perform getppid system call and measure its performance
    gettimeofday(&start, NULL);
    getppid();
    gettimeofday(&end, NULL);

    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // Calculate performance in MiB/s and B/s
    totalDataSizeMB = (double)512 / MEGABYTE; // Assuming a small buffer of 512 bytes for system calls
    performanceMBs = totalDataSizeMB / elapsedTime;
    performanceBs = 512 / elapsedTime;

    printf("System Call: getppid\n");
    printf("Time taken: %.6f seconds\n", elapsedTime);
    printf("Performance: %.2f MiB/s\n", performanceMBs);
    printf("Performance: %.2f B/s\n", performanceBs);
    printf("\n");

    // Close the file descriptor
    close(fd);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage();
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    int blockSize = 1;  // Block size set to 1 byte
    int blockCount;  // Number of blocks set to the file size in bytes

    // Calculate block count based on file size
    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }

    blockCount = fileStat.st_size;
    
    // Measure system call performance
    measureSystemCallPerformance(filename);
    printf("\n");
    printf("\nFinding Performance for 1 Byte Block Size:\n");
    
    printf("\nWARNING!!!!!! THIS WILL TAKE 10 MINUTES TO RUN :\n");
    // Perform test case
    printFileSize(blockSize, blockCount);
    printPerformance(filename, blockSize, blockCount);

    

    return 0;
}
