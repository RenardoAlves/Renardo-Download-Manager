#ifndef PROTOCOL_H
#define PROTOCOL_H

#define PIPE_NAME "\\\\.\\pipe\\dm_manager"
#define MAX_URL_LEN 1024
#define MAX_FILENAME_LEN 256
#define MAX_JOBS 10

typedef enum {
    DM_CMD_ADD,
    DM_CMD_LIST,
    DM_CMD_STOP,
    DM_CMD_EXIT
} CommandType;

typedef struct {
    CommandType type;
    int job_id; // Added for STOP command
    char url[MAX_URL_LEN];
    char filename[MAX_FILENAME_LEN];
} DMMessage;

typedef enum {
    DM_STATUS_PENDING,
    DM_STATUS_DOWNLOADING,
    DM_STATUS_COMPLETED,
    DM_STATUS_FAILED,
    DM_STATUS_CANCELLING,
    DM_STATUS_CANCELLED
} JobStatus;

typedef struct {
    int id;
    char filename[MAX_FILENAME_LEN];
    double progress;
    long long speed;
    JobStatus status;
} JobInfo;

typedef struct {
    int job_count;
    JobInfo jobs[MAX_JOBS];
} StatusResponse;

#endif
