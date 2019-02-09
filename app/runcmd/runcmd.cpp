// This program is based on information I got from:
//   https://support.microsoft.com/en-us/help/190351/how-to-spawn-console-processes-with-redirected-standard-handles
// Some parts of this program may be: Copyright (c) 1998  Microsoft Corporation

#include <windows.h>
#include <stdio.h>

static void startChild(HANDLE out, HANDLE in, HANDLE err) {
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out;
    si.hStdInput  = in;
    si.hStdError  = err;

    FILE* f = fopen("runcmd.txt", "r");
    if (!f) {
        printf("Failed to open 'runcmd.txt' file\n");
        exit(2);
    }
    char cmd[4096];
    fgets(cmd, 4096, f);
    fclose(f);
    cmd[strlen(cmd)-1] = 0;

    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                       NULL, NULL, &si, &pi)) {
        int err = GetLastError();
        printf("Failed to create process, err:%d\n", err);
        exit(2);
    }
    CloseHandle(pi.hThread);
}

static bool stopThread = false;

static DWORD WINAPI handleInput(LPVOID par) {
    HANDLE pipeWrite = (HANDLE)par;
    while (!stopThread) {
        char buf[16384];
        if (!fgets(buf, 16384, stdin))
            break;
        DWORD nBytesRead = strlen(buf);
        DWORD written;
        if (!WriteFile(pipeWrite, buf, nBytesRead, &written, NULL))
            break;
        if (!strcmp("quit\n", buf))
            break;
    }
    return 1;
}

static void handleOutput(HANDLE pipeRead) {
    char buffer[4096];
    while (true) {
        DWORD nRead;
        if (!ReadFile(pipeRead, buffer, sizeof(buffer),
                      &nRead, NULL) || !nRead)
            break;
        fwrite(buffer, 1, nRead, stdout);
        fflush(stdout);
    }
}

int main(int argc, char* argv[]) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE outReadTmp, outWrite;
    CreatePipe(&outReadTmp, &outWrite, &sa, 0);

    HANDLE errWrite;
    DuplicateHandle(GetCurrentProcess(), outWrite, GetCurrentProcess(),
                    &errWrite, 0, TRUE, DUPLICATE_SAME_ACCESS);

    HANDLE inRead, inWriteTmp;
    CreatePipe(&inRead, &inWriteTmp, &sa, 0);

    HANDLE outRead;
    DuplicateHandle(GetCurrentProcess(), outReadTmp, GetCurrentProcess(),
                    &outRead, 0, FALSE, DUPLICATE_SAME_ACCESS);
    HANDLE inWrite;
    DuplicateHandle(GetCurrentProcess(), inWriteTmp, GetCurrentProcess(),
                    &inWrite, 0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(outReadTmp);
    CloseHandle(inWriteTmp);

    startChild(outWrite, inRead, errWrite);

    CloseHandle(outWrite);
    CloseHandle(inRead);
    CloseHandle(errWrite);

    DWORD tId;
    HANDLE hThread = CreateThread(NULL, 0, handleInput,
                                  (LPVOID)inWrite, 0, &tId);
    handleOutput(outRead);

    stopThread = true;
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(outRead);
    CloseHandle(inWrite);
}
