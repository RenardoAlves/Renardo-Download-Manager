#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <stdbool.h>
#include "protocol.h"
#include "downloader.h"

StatusResponse global_status = {0};
HANDLE status_mutex;

// Thread worker argument
typedef struct {
    char url[MAX_URL_LEN];
    char filename[MAX_FILENAME_LEN];
    int job_index;
} ThreadArgs;

unsigned __stdcall worker_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    
    JobInfo *info = NULL;
    
    WaitForSingleObject(status_mutex, INFINITE);
    info = &global_status.jobs[args->job_index];
    ReleaseMutex(status_mutex);

    download_file(args->url, args->filename, info);

    free(args);
    return 0;
}

void handle_client(HANDLE pipe) {
    DMMessage msg;
    DWORD read;

    if (ReadFile(pipe, &msg, sizeof(msg), &read, NULL)) {
        if (msg.type == DM_CMD_ADD) {
            WaitForSingleObject(status_mutex, INFINITE);
            if (global_status.job_count < MAX_JOBS) {
                int idx = global_status.job_count++;
                JobInfo *info = &global_status.jobs[idx];
                info->id = idx + 1;
                info->status = DM_STATUS_PENDING;
                info->progress = 0;
                
                ThreadArgs *args = malloc(sizeof(ThreadArgs));
                strncpy(args->url, msg.url, MAX_URL_LEN);
                strncpy(args->filename, msg.filename, MAX_FILENAME_LEN);
                args->job_index = idx;
                
                _beginthreadex(NULL, 0, worker_thread, args, 0, NULL);
                printf("Added job: %s\n", msg.url);
            }
            ReleaseMutex(status_mutex);
        } else if (msg.type == DM_CMD_LIST) {
            DWORD written;
            WaitForSingleObject(status_mutex, INFINITE);
            WriteFile(pipe, &global_status, sizeof(global_status), &written, NULL);
            ReleaseMutex(status_mutex);
        } else if (msg.type == DM_CMD_STOP) {
            WaitForSingleObject(status_mutex, INFINITE);
            for (int i = 0; i < global_status.job_count; i++) {
                if (global_status.jobs[i].id == msg.job_id) {
                    if (global_status.jobs[i].status == DM_STATUS_DOWNLOADING || 
                        global_status.jobs[i].status == DM_STATUS_PENDING) {
                        global_status.jobs[i].status = DM_STATUS_CANCELLING;
                        printf("Cancelling job %d...\n", msg.job_id);
                    }
                    break;
                }
            }
            ReleaseMutex(status_mutex);
        }
    }
    DisconnectNamedPipe(pipe);
}

#include <stdlib.h>

int daemon_main() {
    status_mutex = CreateMutex(NULL, FALSE, NULL);
    printf("Download Manager Daemon started.\n");
    printf("Listening on %s\n", PIPE_NAME);

    while (true) {
        HANDLE pipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            sizeof(StatusResponse),
            sizeof(DMMessage),
            0,
            NULL
        );

        if (pipe == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Failed to create pipe.\n");
            return 1;
        }

        if (ConnectNamedPipe(pipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            handle_client(pipe);
        }

        CloseHandle(pipe);
    }

    return 0;
}
