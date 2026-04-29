#include <stdio.h>

#include "downloader.h"

int main (int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <URL>\n", argv[0]);
        return 1;
    }

    return download_file(argv[1]);
}