#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>

#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE * KILOBYTE)

void printUsage() {
    printf("Usage: ./performance_measurement <filename>\n");
}

void xorBuffer(char* buffer, int size) {
    for (int i = 0; i < size; ++i) {
        buffer[i] ^= 0xFF;  // adding a padding with xor to avoid overflows.
    }
}

void clearDiskCache(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for clearing caches");
        exit(EXIT_FAILURE);
    }

    // Advise the kernel about the expected access pattern (sequential)
    if (posix_madvise(NULL, 0, MADV_SEQUENTIAL) == -1) {
        perror("Error advising kernel");
        exit(EXIT_FAILURE);
    }

    close(fd);
}

double measureReadTime(const char* filename, int block_size, int block_count, int useCache) {
    int flags = O_RDONLY;
    if (!useCache) {
        // Clear disk cache before non-cached reads
        clearDiskCache(filename);
    }

    int fd = open(filename, flags);
    if (fd == -1) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    // Allocate buffer
    char* buffer = malloc(block_size);
    if (buffer == NULL) {
        perror("Error allocating buffer");
        exit(EXIT_FAILURE);
    }

    ssize_t bytesRead;

    clock_t start, end;
    double totalTime = 0;

    for (int i = 0; i < block_count; ++i) {
        start = clock();

        bytesRead = read(fd, buffer, block_size);
        if (!useCache) {
            xorBuffer(buffer, block_size);
        }

        end = clock();
        double elapsedTime = ((double)(end - start)) / CLOCKS_PER_SEC;

        if (bytesRead == -1) {
            perror("Error reading from file");
            exit(EXIT_FAILURE);
        }

        totalTime += elapsedTime;
    }

    free(buffer);
    close(fd);

    return totalTime;
}

void printFileSize(int block_size, int block_count) {
    double fileSizeKB = (double)block_size * block_count / KILOBYTE;
    double fileSizeMB = fileSizeKB / KILOBYTE;

    printf("Block Size : %d , Block count: %d blocks, %.2f KB, %.2f MB\n", block_size, block_count, fileSizeKB, fileSizeMB);
}

void printPerformance(const char* filename, int block_size, int block_count, int useCache) {
    double totalTime = measureReadTime(filename, block_size, block_count, useCache);

    // Calculate performance in MiB/s
    double totalDataSizeMB = (double)block_size * block_count / MEGABYTE;
    double performance = totalDataSizeMB / totalTime;

    printf("Time taken to read (%s): %.2f seconds\n", (useCache ? "Cached" : "Non-cached"), totalTime);
    printf("Performance: %.2f MiB/s\n", performance);
}

void runTestCases(const char* filename, int useCache) {
    int blockSizes[] = {512, 1024, 1028, 1400, 1424,1600, 1720, 1800, 2000, 2048, 2400};
    int numBlockSizes = sizeof(blockSizes) / sizeof(blockSizes[0]);

    if (useCache) {
        printf("\nBlock Size\tCached Performance (MiB/s)\n\n");
    } else {
        printf("\nBlock Size\tNon-cached Performance (MiB/s)\n\n");
    }

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
        printPerformance(filename, block_size, block_count, useCache);
        printf("\n\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage();
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];

    printf("\nTest case to find the performance for different block sizes in MiB/s with Cache:\n");
    runTestCases(filename, 1);

    printf("\nTest case to find the performance for different block sizes in MiB/s Without Cache:\n");
    runTestCases(filename, 0);

    return 0;
}
