#include "Windows.h"
#include <cstdio>
#include <synchapi.h>

int main(void) {
    printf("Starting semaphore notification test...\n");
    auto sem = CreateSemaphoreA(NULL, 0, 1, "Global\\test_semaphore");
    if (sem == NULL) {
        printf("Failed to create semaphore.\n");
        printf("Error code: %lu\n", GetLastError());
        return 1; // Failed to create semaphore
    }
    printf("Semaphore created successfully.\n");
    for (int i=0; i < 3; i++) {
        
        Sleep(5000);
        printf("Releasing semaphore...\n");
        ReleaseSemaphore(sem, 1, NULL);
    }
    printf("Finished releasing semaphore.\n");
    CloseHandle(sem);
    printf("Semaphore closed successfully.\n");
    printf("Semaphore notification test completed.\n");
    return 0;
}