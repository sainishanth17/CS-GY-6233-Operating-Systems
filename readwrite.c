#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

void printUsage() {
    printf("Usage: ./run <filename> [-r|-w] <block_size> <block_count>\n");
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void readFile(const char* filename, int block_size, int block_count) {
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

        write(STDOUT_FILENO, buffer, bytesRead);
    }

    printf("\nRead %zd characters in total in %.2f seconds\n", (ssize_t) block_count * block_size, totalTime);

    close(fd);
}

void writeFile(const char* filename, int block_size, int block_count) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    char buffer[block_size];

    printf("Enter data to write to the file:\n");
    for (int i = 0; i < block_count; ++i) {
        printf("Block %d: ", i + 1);
        fflush(stdout);

        ssize_t bytesRead = read(STDIN_FILENO, buffer, block_size);
        if (bytesRead == -1) {
            perror("Error reading user input");
            exit(EXIT_FAILURE);
        }

        clearInputBuffer(); // Clear the input buffer

        ssize_t bytesWritten = write(fd, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("Error writing to file");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printUsage();
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    const char* mode = argv[2];
    int block_size = atoi(argv[3]);
    int block_count = atoi(argv[4]);

    if (strcmp(mode, "-r") == 0) {
        readFile(filename, block_size, block_count);
    } else if (strcmp(mode, "-w") == 0) {
        writeFile(filename, block_size, block_count);
    } else {
        fprintf(stderr, "Invalid mode. Use -r for reading or -w for writing.\n");
        printUsage();
        return EXIT_FAILURE;
    }

    return 0;
}
