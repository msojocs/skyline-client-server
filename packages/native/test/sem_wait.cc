#include "Windows.h"
#include <cstdio>
#include <synchapi.h>

int main(void) {
    printf("Starting semaphore wait test...\n");
    auto sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "Global\\test_semaphore");
    if (sem == NULL) {
        printf("Failed to open semaphore.\n");
        // error info
        printf("Error code: %lu\n", GetLastError());
        return 1; // Failed to open semaphore
    }
    printf("Semaphore opened successfully.\n");
    printf("Waiting for semaphore...\n");
    if (WaitForSingleObject(sem, INFINITE) == WAIT_OBJECT_0) {
        printf("Semaphore acquired successfully.\n");
    } else {
        printf("Failed to acquire semaphore.\n");
        CloseHandle(sem);
        return 1; // Failed to acquire semaphore
    }
    printf("Finished waiting for semaphore.\n");
    CloseHandle(sem);
    return 0;
}