#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "downloader.h"

struct DownloadContext {
    FILE *fp;
    char filepath[1024];
    int opened;
    char deduced_filename[256];
    int name_is_fixed;
};

// helper to extract a fallback filename from a URL
static void extract_filename_from_url(const char *url, char *out, size_t out_len) {
    const char *query = strchr(url, '?');
    size_t url_len = query ? (size_t)(query - url) : strlen(url);

    const char *last_slash = NULL;
    for (size_t i = 0; i < url_len; i++) {
        if (url[i] == '/') last_slash = url + i;
    }

    if (last_slash && last_slash + 1 < url + url_len) {
        size_t name_len = (url + url_len) - (last_slash + 1);
        if (name_len >= out_len) name_len = out_len - 1;
        strncpy(out, last_slash + 1, name_len);
        out[name_len] = '\0';
    } else {
        strncpy(out, "downloaded_file.bin", out_len);
        out[out_len - 1] = '\0';
    }
}

static int starts_with_case_insensitive(const char *str, const char *prefix) {
    while (*prefix) {
        char c1 = *str >= 'A' && *str <= 'Z' ? *str + 32 : *str;
        char c2 = *prefix >= 'A' && *prefix <= 'Z' ? *prefix + 32 : *prefix;
        if (c1 != c2) return 0;
        str++;
        prefix++;
    }
    return 1;
}

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t numbytes = size * nitems;
    struct DownloadContext *ctx = (struct DownloadContext *)userdata;

    // If user provided a name explicitly, we do not overwrite it
    if (ctx->name_is_fixed) return numbytes;

    if (starts_with_case_insensitive(buffer, "Content-Disposition:")) {
        const char *fn_ptr = strstr(buffer, "filename=");
        if (!fn_ptr) fn_ptr = strstr(buffer, "FILENAME=");
        if (!fn_ptr) fn_ptr = strstr(buffer, "Filename=");

        if (fn_ptr) {
            fn_ptr += 9; // strlen("filename=")
            int in_quotes = 0;
            if (*fn_ptr == '"') {
                in_quotes = 1;
                fn_ptr++;
            }
            size_t i = 0;
            while (i < sizeof(ctx->deduced_filename) - 1 && *fn_ptr != '\0' && *fn_ptr != '\r' && *fn_ptr != '\n') {
                if (in_quotes && *fn_ptr == '"') break;
                if (!in_quotes && (*fn_ptr == ';' || *fn_ptr == ' ')) break;
                ctx->deduced_filename[i++] = *fn_ptr++;
            }
            ctx->deduced_filename[i] = '\0';
        }
    }
    return numbytes;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
    struct DownloadContext *ctx = (struct DownloadContext *)userdata;
    
    if (!ctx->opened) {
        // Prepend ../downloads/ to the deduced or provided filename
        snprintf(ctx->filepath, sizeof(ctx->filepath), "../downloads/%s", ctx->deduced_filename);
        printf("\nSaving to: %s\n", ctx->filepath);
        
        ctx->fp = fopen(ctx->filepath, "wb");
        if (!ctx->fp) {
            fprintf(stderr, "\nError: Could not create file %s. Does the ../downloads/ folder exist?\n", ctx->filepath);
            return 0; // Aborts the download
        }
        ctx->opened = 1;
    }

    size_t written = fwrite(ptr, size, nmemb, ctx->fp);
    return written;
}

int download_file(const char *url, const char *provided_filename) {
    CURL *curl;
    CURLcode res;
    struct DownloadContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    // Setup initial filename
    if (provided_filename && strlen(provided_filename) > 0) {
        strncpy(ctx.deduced_filename, provided_filename, sizeof(ctx.deduced_filename) - 1);
        ctx.name_is_fixed = 1;
    } else {
        extract_filename_from_url(url, ctx.deduced_filename, sizeof(ctx.deduced_filename));
        ctx.name_is_fixed = 0;
    }

    printf("Downloading: %s\n", url);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        // Header parsing
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &ctx);

        // Body writing
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
        
        // Settings
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "\nDownload failed: %s\n", curl_easy_strerror(res));
            if (ctx.opened) {
                fclose(ctx.fp);
                remove(ctx.filepath); // Delete the partial file
            }
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Failed to initialize curl\n");
        return -1;
    }

    if (ctx.opened) {
        fclose(ctx.fp);
        printf("\nDownload completed successfully.\n");
    } else {
        printf("\nDownload failed or empty file.\n");
    }
    
    return 0;
}