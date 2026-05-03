#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

#include "downloader.h"

int main(int argc, char *argv[]) {

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (argc < 2 || argc > 3) {
        printf("Usage: %s <URL> [filename]\n", argv[0]);
        return 1;
    }

    const char *url = argv[1];
    const char *filename = NULL;

    if (argc == 3) {
        filename = argv[2];
    }

    int result = download_file(url, filename);

    curl_global_cleanup();

    return result;
}