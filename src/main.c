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

void show_help(const char *exe) {
    printf("Usage:\n");
    printf("  %s daemon         - Start the background manager\n", exe);
    printf("  %s add <URL> [fn] - Add a new download\n", exe);
    printf("  %s list           - Show active downloads\n", exe);
    printf("  %s stop <id>      - Cancel a download\n", exe);
    printf("  %s monitor        - Show real time tracking of the downloads\n", exe);
}

int send_command(DMMessage *msg) {
    HANDLE pipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Could not connect to daemon. Is it running? (Try: dm daemon)\n");
        return 1;
    }

    DWORD written;
    WriteFile(pipe, msg, sizeof(DMMessage), &written, NULL);
    
    if (msg->type == DM_CMD_LIST) {
        StatusResponse resp;
        DWORD read;
        if (ReadFile(pipe, &resp, sizeof(resp), &read, NULL)) {
            printf("\n--- Active Downloads ---\n");
            for (int i = 0; i < resp.job_count; i++) {
                JobInfo *j = &resp.jobs[i];
                const char *st = (j->status == DM_STATUS_DOWNLOADING) ? "DOWNLOADING" : 
                                 (j->status == DM_STATUS_COMPLETED) ? "COMPLETED" :
                                 (j->status == DM_STATUS_FAILED) ? "FAILED" :
                                 (j->status == DM_STATUS_CANCELLING) ? "CANCELLING" :
                                 (j->status == DM_STATUS_CANCELLED) ? "CANCELLED" : "PENDING";
                printf("[%d] %-20s | %6.2f%% | %s\n", j->id, j->filename, j->progress, st);
            }
            if (resp.job_count == 0) printf("No downloads active.\n");
        }
    }

    CloseHandle(pipe);
    return 0;
}

void print_progress_bar(double progress) {
    int width = 30;
    int pos = (int)(width * (progress / 100.0));
    printf("[");
    for (int i = 0; i < width; i++) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %6.2f%%", progress);
}

void monitor_ui() {
    DMMessage msg = {0};
    msg.type = DM_CMD_LIST;
    system("cls"); // Limpa a tela inicial

    while (1) {
        printf("\033[H"); // Move o cursor para o topo (evita piscar a tela)
        printf("=== DM REAL-TIME MONITOR === (Pressione Ctrl+C para sair)\n\n");
        send_command(&msg);
        Sleep(500); // Atualiza a cada meio segundo
    }
}


int main(int argc, char *argv[]) {
#ifdef _WIN32
    disable_quick_edit();
#endif

    if (argc < 2) {
        show_help(argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char *cmd = argv[1];

    if (strcmp(cmd, "daemon") == 0) {
        // Run the daemon logic (we'll need to call it from here or link it)
        extern int daemon_main();
        return daemon_main();
    } 
    else if (strcmp(cmd, "add") == 0) {
        if (argc < 3) {
            printf("Usage: %s add <URL> [filename]\n", argv[0]);
            return 1;
        }
        DMMessage msg = {0};
        msg.type = DM_CMD_ADD;
        strncpy(msg.url, argv[2], MAX_URL_LEN);
        if (argc == 4) strncpy(msg.filename, argv[3], MAX_FILENAME_LEN);
        return send_command(&msg);
    }
    else if (strcmp(cmd, "list") == 0) {
        DMMessage msg = {0};
        msg.type = DM_CMD_LIST;
        return send_command(&msg);
    }
    else if (strcmp(cmd, "stop") == 0) {
        if (argc < 3) {
            printf("Usage: %s stop <id>\n", argv[0]);
            return 1;
        }
        DMMessage msg = {0};
        msg.type = DM_CMD_STOP;
        msg.job_id = atoi(argv[2]);
        return send_command(&msg);
    }
    else if (strcmp(cmd, "monitor") == 0) {
        monitor_ui();
        return 0;
    }
    else {
        show_help(argv[0]);
    }

    curl_global_cleanup();
    return 0;
}