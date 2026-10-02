import mmap
import win32event

SHM_NAME = "/ipc_shm"
SHM_SIZE = 1008
SEMAPHORE_READ_NAME = "/sem_read"
SEMAPHORE_WRITE_NAME = "/sem_write"

def shm_init():
    hMapFile = mmap.mmap(-1, SHM_SIZE, SHM_NAME, mmap.ACCESS_DEFAULT)
    hSemaphoreRead = win32event.CreateEvent(None, False, True, SEMAPHORE_READ_NAME)
    hSemaphoreWrite = win32event.CreateEvent(None, False, False, SEMAPHORE_WRITE_NAME)

    return hMapFile, hSemaphoreRead, hSemaphoreWrite

def shm_read(shm:mmap.mmap) -> bytes:
    msg = shm.read(SHM_SIZE).rstrip(b'\x00')
    shm.seek(0)

    return msg

def shm_write(shm:mmap.mmap, message:bytes, semRead:int, semWrite:int) -> bool:
    if len(message) > SHM_SIZE:
        print("Message too big")
        return False

    if win32event.WaitForSingleObject(semRead, 0) != win32event.WAIT_OBJECT_0:
        return False

    shm.seek(0)
    shm.write(message + b"\x00" * (SHM_SIZE - len(message)))
    shm.flush()
    shm.seek(0)

    win32event.SetEvent(semWrite)
    return True

def shm_release(shm:mmap.mmap):
    shm.close()