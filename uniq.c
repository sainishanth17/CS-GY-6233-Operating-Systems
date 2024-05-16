///AUTHOR SAI NISHANTH
#include "types.h"
#include "user.h"

#define MAXLINE 512

int main(int argc, char *argv[]) {
    char line[MAXLINE];
    char prev_line[MAXLINE] = "";
    int fd = -1; 

    int count_flag = 0; 
    int duplicate_flag = 0; 
    int ignore_case_flag = 0;

    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            count_flag = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            duplicate_flag = 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            ignore_case_flag = 1;
        } else {
            if (fd >= 0) {
                printf(2, "uniq: cannot specify multiple files\n");
                exit();
            }
            fd = open(argv[i], 0);
            if (fd < 0) {
                printf(2, "uniq: cannot open %s\n", argv[i]);
                exit();
            }
        }
    }

    if (fd < 0) {
        fd = 0;
    }

    int is_first_line = 1;
    int line_count = 1; 
    int is_duplicate = 0; 

    while (read(fd, line, sizeof(line)) > 0) {
        if (ignore_case_flag) {
            for (i = 0; line[i]; i++) {
                if (line[i] >= 'A' && line[i] <= 'Z') {
                    line[i] += 32; 
                }
                if (prev_line[i] >= 'A' && prev_line[i] <= 'Z') {
                    prev_line[i] += 32; 
                }
            }
        }

        int is_identical = strcmp(prev_line, line) == 0;

        if ((is_first_line || (!is_identical)) && (!is_duplicate || duplicate_flag)) {
            if (count_flag) {
                printf(1, "%7d %s", line_count, prev_line);
                line_count = 1;
            } else {
                printf(1, "%s", prev_line);
            }
        } else if (is_identical && count_flag) {
            line_count++;
        }

        is_first_line = 0;
        if (is_identical) {
            is_duplicate = 1;
        } else {
            is_duplicate = 0;
        }
        strcpy(prev_line, line);
    }

    if (count_flag) {
        printf(1, "%7d %s", line_count, prev_line);
    } else if (!duplicate_flag) {
        printf(1, "%s", prev_line);
    }

    close(fd);
    exit();
}
