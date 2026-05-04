#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "protocol.h"
#include "downloader.h"
#include "network_utils.h"

StatusResponse global_status = {0};
pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char url[MAX_URL_LEN];
    char filename[MAX_FILENAME_LEN];
    int job_index;
} ThreadArgs;

void* worker_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    
    pthread_mutex_lock(&status_mutex);
    JobInfo *info = &global_status.jobs[args->job_index];
    pthread_mutex_unlock(&status_mutex);

    download_file(args->url, args->filename, info);

    free(args);
    return NULL;
}

void handle_client(socket_t client_sock) {
    DMMessage msg;
    if (net_recv(client_sock, &msg, sizeof(msg)) > 0) {
        if (msg.type == DM_CMD_ADD) {
            pthread_mutex_lock(&status_mutex);
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
                
                pthread_t tid;
                pthread_create(&tid, NULL, worker_thread, args);
                pthread_detach(tid);
                printf("Added job: %s\n", msg.url);
            }
            pthread_mutex_unlock(&status_mutex);
        } else if (msg.type == DM_CMD_LIST) {
            pthread_mutex_lock(&status_mutex);
            net_send(client_sock, &global_status, sizeof(global_status));
            pthread_mutex_unlock(&status_mutex);
        } else if (msg.type == DM_CMD_STOP) {
            pthread_mutex_lock(&status_mutex);
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
            pthread_mutex_unlock(&status_mutex);
        }
    }
    net_close(client_sock);
}

int daemon_main() {
    net_init();

    socket_t server_sock = net_create_server(DEFAULT_PORT);
    if (server_sock == INVALID_SOCKET) {
        printf("Failed to start server on port %d\n", DEFAULT_PORT);
        return 1;
    }

    printf("Download Manager Daemon started on port %d.\n", DEFAULT_PORT);

    while (true) {
        socket_t client_sock = net_accept(server_sock);
        if (client_sock != INVALID_SOCKET) {
            handle_client(client_sock);
        }
    }

    net_close(server_sock);
    net_cleanup();
    return 0;
}
