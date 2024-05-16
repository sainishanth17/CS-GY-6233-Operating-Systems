#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE * KILOBYTE)

void printUsage() {
    printf("Usage: ./performance_measurement <filename>\n");
}

double measureReadTime(const char* filename, int block_size, int block_count) {
    int fd = open(filename, O_RDONLY | O_APPEND);
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

    printf("Block Size : %d , Block count: %d blocks, %.2f KB, %.2f MB\n", block_size, block_count, fileSizeKB, fileSizeMB);
}

void printPerformance(const char* filename, int block_size, int block_count) {
    double totalTime = measureReadTime(filename, block_size, block_count);

    // Calculate performance in MiB/s
    double totalDataSizeMB = (double)block_size * block_count / MEGABYTE;
    double performance = totalDataSizeMB / totalTime;

    printf("Time taken to read: %.2f seconds\n", totalTime);
    printf("Performance: %.2f MiB/s\n", performance);
}

void runTestCases(const char* filename) {
    int blockSizes[] = {512, 1024, 1028, 1400, 1424,1600, 1720, 1800, 2000, 2048, 2400};
    int numBlockSizes = sizeof(blockSizes) / sizeof(blockSizes[0]);

    printf("\nBlock Size\tPerformance (MiB/s)\n\n");

    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numBlockSizes; ++i) {
        int block_size = blockSizes[i];
        int block_count = fileStat.st_size / block_size;

        // Perform test case
        printFileSize(block_size, block_count);
        printPerformance(filename, block_size, block_count);
        printf("\n\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage();
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    int defaultBlockSize = 512;  // Default block size (adjust as needed)

    printf("\nFinding Performance for the Optimal File Size :\n\n");

    // Calculate block count based on file size and block size
    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }

    int defaultBlockCount = fileStat.st_size / defaultBlockSize;

    // Perform default test case
    printFileSize(defaultBlockSize, defaultBlockCount);
    printPerformance(filename, defaultBlockSize, defaultBlockCount);

    printf("\nTest case to find the performance for different block sizes in MiB/s:\n");
    runTestCases(filename);

    return 0;
}
