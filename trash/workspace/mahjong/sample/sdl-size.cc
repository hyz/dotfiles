#include <stdlib.h>
#include "SDL.h"

int main(int argc, char* const argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0) { 
        //ERR_EXIT("Unable to init SDL: %s", SDL_GetError());
    }
    SDL_DisplayMode dm; //SDL_GetNumVideoDisplays()
    SDL_GetCurrentDisplayMode(0, &dm);
    SDL_Window* window_;
    if (argc > 1)
        window_ = SDL_CreateWindow("Mahjong", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 0,0, SDL_WINDOW_RESIZABLE|SDL_WINDOW_MAXIMIZED);
    else
        window_ = SDL_CreateWindow("Mahjong", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 0,0, SDL_WINDOW_RESIZABLE|SDL_WINDOW_MAXIMIZED);
    //window_ = SDL_CreateWindow("Mahjong", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, squa_.w,squa_.h, 0);//SDL_WINDOW_BORDERLESS,SDL_WINDOW_FULLSCREEN_DESKTOP,SDL_WINDOW_FULLSCREEN

    auto renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(renderer_, 1024, 768);
    {
        printf("dm %dx%d", dm.w, dm.h);

        int w,h;
        SDL_GetWindowSize(window_, &w, &h);
        printf("\tws %dx%d", w, h);
        SDL_GetRendererOutputSize(renderer_, &w, &h);
        printf("\tros %dx%d", w, h);
        //SDL_Rect rvp;
        //SDL_RendererGetViewport(renderer_, &rvp);
        //printf("\tros %dx%d+%d+%d", rvp.w, rvp.h, rvp.x, rvp.y);
    }
}

