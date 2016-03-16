#include <stdlib.h>
#include <memory>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"

#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    //fflush(stderr);
}
#define ENSURE(c, ...) if(!(c))ERR_EXIT(__VA_ARGS__)

// Screen surface
SDL_Window *gWindow; //SDL_Surface *gScreen;
SDL_Surface *gSurface;
SDL_Renderer* gRenderer;
SDL_Texture* gTexture;

#define TILE_WIDTH                42
#define TILE_HEIGHT_CONCEALED     76
#define TILE_HEIGHT_SHOWN         60
#define TILE_HEIGHT_WALL          60
#define TILE_HEIGHT_COMPUTER      70

//void CGeneral::DrawTile(const CTile &t, int x, int y, int dir, int size)
//{
//   SDL_Rect dstrect, dstrect2;
//
//   dstrect.x = x;
//   dstrect.y = y;
//
//   if (dir <= PLAYER_CONCEALED) {
//      // shown tile
//      dstrect.w = dstrect2.w = TILE_WIDTH;
//      dstrect.h = dstrect2.h = ((dir < PLAYER_CONCEALED) ?
//         TILE_HEIGHT_SHOWN : TILE_HEIGHT_CONCEALED);
//
//      if (t.GetSuit() == TILESUIT_DRAGON) {
//         dstrect2.x = (t.index2() + 4) * TILE_WIDTH;
//         dstrect2.y = 3 * dstrect.h;
//      } else {
//         dstrect2.x = t.index2() * TILE_WIDTH;
//         dstrect2.y = t.index1() * dstrect.h;
//      }
//
//      if (dir < PLAYER_CONCEALED) {
//         dstrect2.y += TILE_HEIGHT_CONCEALED * 4;
//      }
//   } else if (dir == COMPUTER_CONCEALED) {
//      // concealed tile
//      dstrect2.x = 0;
//      dstrect2.y = TILE_HEIGHT_SHOWN * 4 + TILE_HEIGHT_CONCEALED * 4;
//      dstrect2.w = dstrect.w = TILE_WIDTH;
//      dstrect2.h = dstrect.h = TILE_HEIGHT_COMPUTER;
//   } else if (dir == WALL_CONCEALED) {
//      dstrect2.x = TILE_WIDTH;
//      dstrect2.y = TILE_HEIGHT_SHOWN * 4 + TILE_HEIGHT_CONCEALED * 4;
//      dstrect2.w = dstrect.w = TILE_WIDTH;
//      dstrect2.h = dstrect.h = TILE_HEIGHT_WALL;
//   }
//
//   // Draw the tile to the screen
//   if (size >= 1) {
//       SDL_RenderCopy(gRenderer, m_imgTiles, &dstrect2, &dstrect);
//      //SDL_BlitSurface(m_imgTiles, &dstrect2, gpScreen, &dstrect);
//   } else {
//      dstrect.w = dstrect.w * 7 / 10;
//      dstrect.h = dstrect.h * 7 / 10;
//      SDL_RenderCopy(gRenderer, m_imgTiles, &dstrect2, &dstrect);
//      //UTIL_ScaleBlit(m_imgTiles, &dstrect2, gpScreen, &dstrect);
//   }
//}
//
//void CGeneral::DrawTiles(const CTile t[], int num, int x, int y, int dir, int size)
//{
//   int i;
//
//   if (dir == COMPUTER_SHOWN) {
//      // Draw computer's tiles in reverse order
//      for (i = num - 1; i >= 0; i--) {
//         DrawTile(t[i], (int)(x + (num - i - 1) * TILE_WIDTH * (size ? 1 : 0.7)),
//            y, dir, size);
//      }
//   } else {
//      // Otherwise draw in normal order
//      for (i = 0; i < num; i++) {
//         DrawTile(t[i], (int)(x + i * TILE_WIDTH * (size ? 1 : 0.7)), y, dir, size);
//      }
//   }
//}

void drawTilesRow(SDL_Texture* tils, int w, int h, int y)
{
    int w0, h0;
    SDL_QueryTexture(tils, 0, 0, &w0, &h0);

    SDL_Rect srcRect = {0,y,w,h};
    while (srcRect.x + w < w0) {
        SDL_RenderCopy(gRenderer, tils, &srcRect, &srcRect);
        srcRect.x += w;
    }
}

void drawTiles(SDL_Texture* tils)
{
    drawTilesRow(tils, TILE_WIDTH, TILE_HEIGHT_COMPUTER, 0);
    //SDL_UpdateTexture(gTexture, NULL, gSurface->pixels, gSurface->pitch);
    //SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
}

SDL_Texture *CreateTextureFromImageFile(const char *filename)
{
    SDL_Surface *pic = IMG_Load(filename);
    ENSURE(pic, "IMG_Load: %s", SDL_GetError());
    //std::unique_ptr<SDL_Surface,decltype(& SDL_FreeSurface)> xfree(pic, SDL_FreeSurface);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(gRenderer, pic);
    SDL_FreeSurface(pic);
    return tex;
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0) { 
        ERR_EXIT("Unable to init SDL: %s", SDL_GetError());
    }
    atexit(SDL_Quit);

    gWindow = SDL_CreateWindow("Mahjong"
            , SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 0, 0
            , SDL_WINDOW_FULLSCREEN_DESKTOP/*SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL*/);
    ENSURE(gWindow, "SDL_CreateWindow: %s", SDL_GetError());
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_PRESENTVSYNC);
    ENSURE(gRenderer, "SDL_CreateRenderer: %s", SDL_GetError());
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(gRenderer, 640, 480);

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);

    gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
    //gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 640, 480);

    ENSURE(argc>1, "argc");
    SDL_Texture* tils = CreateTextureFromImageFile(argv[1]);
    //gSurface = SDL_CreateRGBSurface(0, 640, 480, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    //SDL_FillRect(gSurface, &{64,48,64,48}, 0xff);

    while (1) {
        drawTiles(tils);
        SDL_RenderPresent(gRenderer);
        SDL_Delay(10); 

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYUP:
                    switch (int sym = event.key.keysym.sym) {
                        case SDLK_ESCAPE: // If escape is pressed, return (and thus, quit)
                            return 0;
                        default: ;//;drawrect(gSurface, {64,48,64,48}, 0x88ff|((sym*3)<<16)&0xffffff);
                    }
                    break;
                case SDL_QUIT:
                    return(0);
            }
        }
    }
    return 0;
}

