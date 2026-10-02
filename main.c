#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>

#include "ipc/ipc.h"
#include "rendering/types.h"

static volatile bool qPressed = false;
int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;
const int MAX_LINES = 42;
const int FLOAT_PER_LINE = 4;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wPar, LPARAM lPar)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT *)lPar;
        if (wPar == WM_KEYDOWN || WM_SYSKEYDOWN)
        {
            if (kbd->vkCode == 'Q')
            {
                qPressed == true;
            }
        }
    }
}

void SetClickThrough(SDL_Window *window, int enable)
{
    HWND hWnd = (HWND)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        NULL
    );

    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

    if (enable) {
        exStyle |= WS_EX_TRANSPARENT | WS_EX_LAYERED;
    }
    else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }

    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(
        hWnd,
        NULL,
        0, 0, 0, 0,
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_NOZORDER |
        SWP_FRAMECHANGED
    );
}

void DrawCircle(SDL_Renderer *renderer, float X, float Y, float r)
{
    for (int y = -r; y <= r; y++)
    {
        for (int x = -r; x <= r; x++)
        {
            if (x*x + y*y <= r*r)
            {
                SDL_RenderPoint(renderer, x+X, y+Y);
            }
        }
    }
}

int main()
{
    shared_memory shm = {0};
    shm_init(&shm);

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    SDL_Window *win = SDL_CreateWindow("hewwo", 300, 250, SDL_WINDOW_TRANSPARENT | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_FULLSCREEN);

    bool clickthrough = true;
    SetClickThrough(win, clickthrough);

    //HHOOK qKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(win, NULL);

    bool running = true;
    SDL_Event e;
    float *buffer = malloc(sizeof(float) * SHM_SIZE);
    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT || e.key.key == 'q')
            {
                running = false;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        if (shm_read(&shm, buffer)) {
            printf("Recieved data %.2f\n", buffer[0]);
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
        for (size_t i = 0; i < 168; i+=4)
        {
            if (buffer[i] != -1.0f)
            {
                SDL_RenderLine(renderer, buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]);
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
        for (size_t i = 168; i < 252; i+=2)
        {
            if (buffer[i] != -1.0f)
            {
                DrawCircle(renderer, buffer[i], buffer[i+1], 5);
            }
        }

        SDL_RenderPresent(renderer);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    shm_release(&shm);
    return 0;
}