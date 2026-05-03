#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

#include "downloader.h"

#ifdef _WIN32
#include <windows.h>
void disable_quick_edit() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prev_mode;
    GetConsoleMode(hInput, &prev_mode);
    SetConsoleMode(hInput, (prev_mode & ~ENABLE_QUICK_EDIT_MODE) | ENABLE_EXTENDED_FLAGS);
}
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
    disable_quick_edit();
#endif

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