#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <pthread.h>

#define KILOBYTE 1024
#define MEGABYTE (KILOBYTE * KILOBYTE)

struct ThreadData {
    const char* filename;
    int block_size;
    int block_count;
    int useCache;
    double totalTime;
};

void shuffleArray(int arr[], int n) {
    srand(time(NULL));
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void printUsage() {
    printf("Usage: ./fast <filename>\n");
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

void* readThread(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;

    int flags = O_RDONLY;
    if (!data->useCache) {
        clearDiskCache(data->filename);
    }

    int fd = open(data->filename, flags);
    if (fd == -1) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    char* buffer = malloc(data->block_size);
    if (buffer == NULL) {
        perror("Error allocating buffer");
        exit(EXIT_FAILURE);
    }

    ssize_t bytesRead;

    clock_t start, end;

    start = clock();

    for (int i = 0; i < data->block_count; ++i) {
        bytesRead = read(fd, buffer, data->block_size);
        if (!data->useCache) {
            xorBuffer(buffer, data->block_size);
        }

        if (bytesRead == -1) {
            perror("Error reading from file");
            exit(EXIT_FAILURE);
        }
    }

    end = clock();

    free(buffer);
    close(fd);

    data->totalTime = ((double)(end - start)) / CLOCKS_PER_SEC;

    return NULL;
}

double measureReadTimeMultithread(const char* filename, int block_size, int block_count, int useCache) {
    int numThreads = 4; // Adjust the number of threads as needed

    pthread_t threads[numThreads];
    struct ThreadData data[numThreads];

    for (int i = 0; i < numThreads; ++i) {
        data[i].filename = filename;
        data[i].block_size = block_size;
        data[i].block_count = block_count / numThreads;
        data[i].useCache = useCache;
        data[i].totalTime = 0.0;

        pthread_create(&threads[i], NULL, readThread, &data[i]);
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    double totalTime = 0.0;
    for (int i = 0; i < numThreads; ++i) {
        totalTime += data[i].totalTime;
    }

    return totalTime;
}

int printPerformanceMultithread(const char* filename, int useCache) {
    int blockSizes[] = {512, 1024, 1028, 1400, 1424, 1600, 1720, 1800, 2000, 2048, 2400};
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

    int bestBlockSize = 0;
    double bestPerformance = 0.0;

    for (int i = 0; i < numBlockSizes; ++i) {
        int block_size = blockSizes[i];
        int block_count = fileStat.st_size / block_size;

        double totalTime = measureReadTimeMultithread(filename, block_size, block_count, useCache);

        // Calculate performance in MiB/s
        double totalDataSizeMB = (double)block_size * block_count / MEGABYTE;
        double performance = totalDataSizeMB / totalTime;

        printf("Block Size : %d , Block count: %d blocks\n", block_size, block_count);
        printf("Performance: %.2f MiB/s\n", performance);
        printf("\n\n");

        // Update best block size based on performance
        if (performance > bestPerformance) {
            bestPerformance = performance;
            bestBlockSize = block_size;
        }
    }

    return bestBlockSize;
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

void runPerformanceTest(const char* filename, int useCache) {
    int blockSizes[] = {512, 1024, 1028, 1400, 1424,1600, 1720, 1800, 2000, 2048, 2400};
    int numBlockSizes = sizeof(blockSizes) / sizeof(blockSizes[0]);
    shuffleArray(blockSizes, numBlockSizes);
    double bestPerformance = 0.0;
    int bestBlockSize = 0;

    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numBlockSizes; ++i) {
        int block_size = blockSizes[i];
        int block_count = fileStat.st_size / block_size;

        // Perform test case
        double totalTime = measureReadTime(filename, block_size, block_count, useCache);

        // Calculate performance in MiB/s
        double totalDataSizeMB = (double)block_size * block_count / MEGABYTE;
        double performance = totalDataSizeMB / totalTime;

        // Print results for each block size
        printFileSize(block_size, block_count);
        printf("Time taken to read (%s): %.2f seconds\n", (useCache ? "Cached" : "Non-cached"), totalTime);
        printf("Performance: %.2f MiB/s\n", performance);
        printf("\n");

        // Update best performance block size
        if (performance > bestPerformance) {
            bestPerformance = performance;
            bestBlockSize = block_size;
        }
    }

    // Print the best performance block size
    printf("Best Performance Block Size (%s): %d\n", (useCache ? "Cached" : "Non-cached"), bestBlockSize);
    printf("Best Performance: %.2f MiB/s\n\n", bestPerformance);
}

void runGenericTest(const char* filename) {
    int blockSizes[] = {512, 1024, 1028, 1400, 1424,1600, 1720, 1800, 2000, 2048, 2400};
    int numBlockSizes = sizeof(blockSizes) / sizeof(blockSizes[0]);
    shuffleArray(blockSizes, numBlockSizes);
    double bestPerformance = 0.0;
    int bestBlockSize = 0;

    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) {
        perror("Error getting file information");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numBlockSizes; ++i) {
        int block_size = blockSizes[i];
        int block_count = fileStat.st_size / block_size;

        // Perform test case
        double totalTimeCached = measureReadTime(filename, block_size, block_count, 1);
        double totalTimeNonCached = measureReadTime(filename, block_size, block_count, 0);

        // Calculate average performance in MiB/s
        double totalDataSizeMB = (double)block_size * block_count / MEGABYTE;
        double performanceCached = totalDataSizeMB / totalTimeCached;
        double performanceNonCached = totalDataSizeMB / totalTimeNonCached;
        double averagePerformance = (performanceCached + performanceNonCached) / 2;

        // Print results for each block size
        printFileSize(block_size, block_count);
        printf("Cached Performance: %.2f MiB/s\n", performanceCached);
        printf("Non-cached Performance: %.2f MiB/s\n", performanceNonCached);
        printf("Average Performance: %.2f MiB/s\n", averagePerformance);
        printf("\n");

        // Update best performance block size
        if (averagePerformance > bestPerformance) {
            bestPerformance = averagePerformance;
            bestBlockSize = block_size;
        }
    }

    // Print the best performance block size for the generic test
    printf("Best Performance Block Size (Generic Test): %d\n", bestBlockSize);
    printf("Best Performance: %.2f MiB/s\n\n", bestPerformance);
}


void findXORValue(const char* filename, int finalBlockSize, int maxRetries) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for XOR calculation");
        exit(EXIT_FAILURE);
    }

    ssize_t bytesRead;
    unsigned char buffer[1024];  // Adjust buffer size as needed

    // Initialize XOR result
    int xorResult = 0;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        // Update XOR result with offset adjustment
        for (ssize_t i = 0; i < bytesRead; ++i) {
            xorResult ^= buffer[i]; // Adjusting the offset by 1 as we added padding.
        }
    }

    if (bytesRead == -1) {
        perror("Error reading from file for XOR calculation");
        exit(EXIT_FAILURE);
    }

    close(fd);

    if (xorResult == 0 && maxRetries > 0) {
        fprintf(stderr, "Warning: XOR result is zero. Check file content and block size. Retrying...\n");

        // Retry XOR calculation recursively with reduced maxRetries
        findXORValue(filename, finalBlockSize, maxRetries - 1);
        return;  // Exit the function to avoid printing XOR value multiple times
    }

    printf("XOR Value for the file with Block Size %d: %08x\n", finalBlockSize, xorResult);
}



unsigned int xorFile(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    unsigned int result = 0;
    unsigned int buffer;

    while (fread(&buffer, sizeof(unsigned int), 1, file) == 1) {
        result ^= buffer;
    }

    fclose(file);
    return result;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage();
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    int bestBlockSizeCached = 0;
    int bestBlockSizeUncached = 0;
    

    printf("\nTest case to find the best performance block size for Cached Reads:\n");
    runPerformanceTest(filename, 1);

    printf("\nTest case to find the best performance block size for Non-cached Reads:\n");
    runPerformanceTest(filename, 0);
    
    printf("\n\n Let's move ahead and run multiple threads!!!\n\n");
    
 printf("\nTest case to find the best performance block size for Cached Reads (Multithreaded):\n");
    bestBlockSizeCached = printPerformanceMultithread(filename, 1);

    printf("\nTest case to find the best performance block size for Non-cached Reads (Multithreaded):\n");
    bestBlockSizeUncached = printPerformanceMultithread(filename, 0);

    int finalBlockSize = 0;
    if(bestBlockSizeCached > bestBlockSizeUncached){
        printf("\nOverall the best Block Size is %d\n", bestBlockSizeCached);
        finalBlockSize = bestBlockSizeCached;
    }
    else{
        printf("\nOverall the best Block Size is %d \n", bestBlockSizeUncached);
        finalBlockSize = bestBlockSizeUncached;
    }
    
    
    printf("\n\n Let's move ahead and find the XOR Value using the Best Block Size !!!\n\n");
    
    findXORValue(filename, finalBlockSize,4);
    
    
    printf("\n\n Let's find the XOR Value directly with default buffer size !!!\n\n");
    
     unsigned int result = xorFile(filename);

    printf("XOR Value for the entire file: %08x\n", result);
    


    return 0;
}