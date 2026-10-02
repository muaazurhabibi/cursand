#include "ipc.h"

void shm_init(shared_memory *shm)
{
    shm->hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0, SHM_SIZE, SHM_NAME
    );
    if (shm->hMapFile == NULL)
    {
        printf("Couldnt create SHM Map\n");
    }
    
    shm->pBuf = MapViewOfFile(shm->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
    if (shm->pBuf == NULL)
    {
        printf("Couldnt create file map view\n");
    }
    
    shm->hSemaphoreRead = CreateEvent(NULL, FALSE, TRUE, SEMAPHORE_READ_NAME);
    shm->hSemaphoreWrite = CreateEvent(NULL, FALSE, FALSE, SEMAPHORE_WRITE_NAME);
    if (shm->hSemaphoreRead == NULL || shm->hSemaphoreWrite == NULL)
    {
        printf("Coulnt create semaphore operation events\n");
    }
}

boolean shm_read(shared_memory *shm, void *buffer)
{
    if (WaitForSingleObject(shm->hSemaphoreWrite, 0) != WAIT_OBJECT_0) {
        return false;
    }

    //strncpy(buffer, (void *)shm->pBuf, SHM_SIZE);
    memcpy(buffer, (void *)shm->pBuf, SHM_SIZE);

    SetEvent(shm->hSemaphoreRead);
    return true;
}

void shm_write(shared_memory *shm, char *buffer)
{
    WaitForSingleObject(shm->hSemaphoreRead, INFINITE);
    strncpy((char *)shm->pBuf, buffer, SHM_SIZE);
    SetEvent(shm->hSemaphoreWrite);
}

void shm_release(shared_memory *shm)
{
    CloseHandle(shm->hMapFile);
    CloseHandle(shm->hSemaphoreRead);
    CloseHandle(shm->hSemaphoreWrite);
    UnmapViewOfFile(shm->pBuf);
}