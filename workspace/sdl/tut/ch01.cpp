#include <stdlib.h>
#include "SDL.h"

// Screen surface
SDL_Window *gWindow; //SDL_Surface *gScreen;
SDL_Surface *gSurface;
SDL_Renderer* gRenderer;
SDL_Texture* gTexture;

// Simplified interface to SDL's fillrect call
void drawrect(SDL_Surface*s, SDL_Rect r, int color) //void drawrect(int x, int y, int w, int h, int color)
{
  //SDL_Rect r;
  //r.x = x;
  //r.y = y;
  //r.w = w;
  //r.h = h;
  SDL_FillRect(s, &r, color);
}

// Rendering function
void render()
{   
  // clear screen
  //drawrect( {0,0,640,480}, 0);

  // test that the fillrect is working
  //drawrect( {64,48,64,48}, 0xff);

  // update the screen
  //SDL_UpdateRect(gScreen, 0, 0, 640, 480);    

  // don't take all the cpu time
SDL_UpdateTexture(gTexture, NULL, gSurface->pixels, gSurface->pitch);
SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
SDL_RenderPresent(gRenderer);
  SDL_Delay(10); 
}


// Entry point
int main(int argc, char *argv[])
{
  // Initialize SDL's subsystems - in this case, only video.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
  {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }

  // Register SDL_Quit to be called at exit; makes sure things are
  // cleaned up when we quit.
    atexit(SDL_Quit);
    
  // Attempt to create a 640x480 window with 32bit pixels.
    //gScreen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);

    gWindow = SDL_CreateWindow("ch01"
            , SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 0, 0
            , SDL_WINDOW_FULLSCREEN_DESKTOP/*SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL*/);
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_PRESENTVSYNC);
  // If we fail, return error.
    if (gWindow == NULL) {
        fprintf(stderr, "Unable to set up video: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(gRenderer, 640, 480);

gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
//gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 640, 480);

gSurface = SDL_CreateRGBSurface(0, 640, 480, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
drawrect(gSurface, {64,48,64,48}, 0xff);

SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
SDL_RenderClear(gRenderer);
  
  // Main loop: loop forever.
    while (1)
    {
    // Render stuff
        render();

    // Poll for events, and handle the ones we care about.
        SDL_Event event;
        while (SDL_PollEvent(&event)) 
        {
            switch (event.type) 
            {
            case SDL_KEYUP:
                    
        switch (int sym = event.key.keysym.sym)
        {
        case SDLK_ESCAPE: // If escape is pressed, return (and thus, quit)
          return 0;
        default:
            drawrect(gSurface, {64,48,64,48}, 0x88ff|((sym*3)<<16)&0xffffff);
        }
                break;
            case SDL_QUIT:
                return(0);
            }
        }
    }
    return 0;
}
