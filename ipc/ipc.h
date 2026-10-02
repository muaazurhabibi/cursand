#include <windows.h>
#include <stdio.h>
#include <string.h>

#define SHM_NAME "/ipc_shm"
#define SHM_SIZE 1008
#define SEMAPHORE_READ_NAME "/sem_read"
#define SEMAPHORE_WRITE_NAME "/sem_write"

typedef struct {
    HANDLE hMapFile;
    LPCSTR pBuf;
    HANDLE hSemaphoreRead;
    HANDLE hSemaphoreWrite;
} shared_memory;

void shm_init(shared_memory *shm);
boolean shm_read(shared_memory *shm, void *buffer);
void shm_write(shared_memory *shm, char *buffer);
void shm_release(shared_memory *shm);