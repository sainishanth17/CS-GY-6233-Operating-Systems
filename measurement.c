#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

void printUsage() {
    printf("Usage: ./measurement <filename> <block_size>\n");
}

void generateRandomData(char* buffer, int size) {
    for (int i = 0; i < size; ++i) {
        buffer[i] = rand() % 256;  // Fill the buffer with random byte values (0-255)
    }
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
    double fileSizeKB = (double)block_size * block_count / 1024.0;
    double fileSizeMB = fileSizeKB / 1024.0;

    printf("File size: %d blocks, %.2f KB, %.2f MB\n", block_count, fileSizeKB, fileSizeMB);
}

void createFile(const char* filename, int block_size, int block_count) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }

    char buffer[block_size];

    for (int i = 0; i < block_count; ++i) {
        generateRandomData(buffer, block_size);  // Fill the buffer with random data
        ssize_t bytesWritten = write(fd, buffer, block_size);
        if (bytesWritten == -1) {
            perror("Error writing to file");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage();
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    int block_size = atoi(argv[2]);

    // Measure time for initial file size
    struct stat fileStat;
    if (stat(filename, &fileStat) == -1) {
        perror("Error getting file information");
        return EXIT_FAILURE;
    }

    int initialBlockCount = fileStat.st_size / block_size;

    double initialTime = measureReadTime(filename, block_size, initialBlockCount);

    printf("Initial file size: %d blocks\n", initialBlockCount);
    printFileSize(block_size, initialBlockCount);
    printf("Time taken for initial file size: %.2f seconds\n", initialTime);

    int block_count = initialBlockCount;
    double totalTime;

    // Keep doubling the file size until the time is within the specified range
    do {
        block_count *= 2;
        totalTime = measureReadTime(filename, block_size, block_count);
    } while (totalTime < 5.0 || totalTime > 15.0);

    printf("Final file block count : %d blocks\n", block_count);
    printFileSize(block_size, block_count);
    printf("Time taken for final file size: %.2f seconds\n", totalTime);

    // Save the file with the optimal size as "optimal_file_size.file"
    
    printf("\nSaving the file as 'optimal_file_size.file'\n");
    createFile("optimal_file_size.file", block_size, block_count);

    // Now, let's compare with dd command on temp.txt with the final size
    printf("\nComparing with dd command in linux :\n");

    char ddCommand[1024];
    snprintf(ddCommand, sizeof(ddCommand), "dd if=optimal_file_size.file of=/dev/null bs=%d count=%d", block_size, block_count);

    clock_t start_dd, end_dd;
    double time_dd;

    start_dd = clock();
    system(ddCommand);
    end_dd = clock();

    time_dd = ((double)(end_dd - start_dd)) / CLOCKS_PER_SEC;

    return 0;
}
