#include <stdio.h>

#include "downloader.h"

int download_file(const char *url, const char *filename) {
    printf("Downloading: %s to %s\n", url, filename);
    return 0;
}