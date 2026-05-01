#include <stdio.h>
#include <string.h>

#include "downloader.h"

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <URL> [filename]\n", argv[0]);
        return 1;
    }

    const char *url = argv[1];
    const char *filename = NULL;

    if (argc == 3) {
        filename = argv[2];
    } else {
        filename = strrchr(url, '/');
        if (filename && *(filename + 1) != '\0') {
            filename++; // Skip the slash
        } else {
            filename = "downloaded_file"; // Fallback if no slash or slash is at the end
        }
    }

    return download_file(url, filename);
}