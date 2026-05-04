#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "protocol.h"

int download_file(const char *url, const char *filename, JobInfo *info);

#endif