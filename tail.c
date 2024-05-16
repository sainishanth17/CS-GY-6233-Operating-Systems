///AUTHOR METTU SAI NISHANTH
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char buffer[1024];

void printTail(int fileDescriptor, char *filename, int lines) {
  int i, bytesRead, totalLines = 0, lineCount = 0;

  int tempFile = open("temporary", O_CREATE | O_RDWR);

  while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0) {
    write(tempFile, buffer, bytesRead);

    for (i = 0; i <= bytesRead; i++) {
      if (buffer[i] != '\n') {
        continue;
      } else {
        totalLines++;
      }
    }
  }

  close(tempFile);

  if (bytesRead < 0) {
    printf(1, "tail: read error\n");
    exit();
  }

  tempFile = open("temporary", 0);

  while ((bytesRead = read(tempFile, buffer, sizeof(buffer))) > 0) {
    for (i = 0; i < bytesRead; i++) {
      if (lineCount >= (totalLines - lines)) {
        printf(1, "%c", buffer[i]);
      } else if (totalLines < lines) {
        printf(1, "%c", buffer[i]);
      } else if (buffer[i] == '\n') {
        lineCount++;
      }
    }
  }

  close(tempFile);
  unlink("temporary");
}

int main(int argumentCount, char *arguments[]) {
  int index;
  int fileDescriptor = 0;
  int numLines = 10;
  char *fileName;
  char charValue;

  fileName = "";

  if (argumentCount <= 1) {
    printTail(0, "", 10);
    exit();
  } else {
    for (index = 1; index < argumentCount; index++) {
      charValue = *arguments[index];

      if (charValue == '-') {
        arguments[index]++;
        numLines = atoi(arguments[index]++);
      } else {
        if ((fileDescriptor = open(arguments[index], 0)) < 0) {
          printf(1, "tail: cannot open %s\n", arguments[index]);
          exit();
        }
      }
    }

    printTail(fileDescriptor, fileName, numLines);
    close(fileDescriptor);
    exit();
  }
}
