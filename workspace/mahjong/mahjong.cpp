#include <stdlib.h>
#include <memory>
#include <array>
#include <vector>
#include <deque>
#include <algorithm>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
// 万 筒 索 字
// 东 南 西 北 中 发 白
// 顺 刻 将

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

// 万 筒 索 字
// 东 南 西 北 中 发 白
// 顺 刻 将
struct hand : std::array<std::array<int8_t,9>,4>
{
    std::array<int8_t,4> nc;

    hand() : std::array<std::array<int8_t,9>,4>({}) {}
    //int8_t pair_idx = -1;
    //static char* parse(std::array<int8_t,9>& mj, char const* beg, char const* end);
    //static bool parse_file(hand& mj, FILE* fp);
};

struct Mahjong : std::array<hand,4>
{
    enum { Nplayer = 4 }; //enum { East = 0, South, West, North };
    std::deque<int> wall_;
    std::vector<int> discard_;
    int8_t pos_first_hand_ = -1;
    struct Init;

    void distribute(int pos, int nc=13);
    void prepare(int pos) {
        hand& h = (*this)[pos%Nplayer];
        for (int i=0; i < 4; ++i) {
            auto& a = h[i];
            h.nc[i] = std::accumulate(a.begin(), a.end(), 0);
        }
    }

    void take(int pos);
    void discard(int pos);
    void hu(int pos);
    void pen(int pos);
    void gan(int pos);
};

struct Mahjong::Init : std::array<std::vector<int>,4>
{
    Mahjong* thiz;
    int n_ = 0;
    Init(Mahjong* p) : thiz(p) {}

    void wall(int x, std::vector<int> v) {
        ENSURE(x<4, "wall");
        (*this)[x] = std::move(v);
        if (++n_ == 4)
            done();
    }
    void done() {
        for (int x=0; x<4; ++x) {
            auto& v = (*this)[x];
            thiz->wall_.insert(thiz->wall_.end(), v.begin(), v.end());
        }
    }
};

void Mahjong::distribute(int pos, int nc)
{
    pos_first_hand_ = pos;
    nc *= Nplayer;
    while (nc > Nplayer) {
        for (int n=0; n < 4; ++n)
            take(pos);
        ++pos;
        nc -= 4;
    }
    ENSURE(pos%Nplayer==pos_first_hand_, "distribute pos");
    while (nc > 0) {
        take(pos);
        prepare(pos);
        ++pos;
        nc -= 1;
    }
    ENSURE(pos%Nplayer==pos_first_hand_, "distribute pos");
}

void Mahjong::take(int pos)
{
    hand& h = (*this)[pos%Nplayer];
    int code = wall_.front();
    int x = (code >> 4) & 0x0f;
    int y = code & 0x0f;
    h[x][y]++;
    wall_.pop_front();
}

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
//       SDL_RenderCopy(renderer_, m_imgTiles, &dstrect2, &dstrect);
//      //SDL_BlitSurface(m_imgTiles, &dstrect2, gpScreen, &dstrect);
//   } else {
//      dstrect.w = dstrect.w * 7 / 10;
//      dstrect.h = dstrect.h * 7 / 10;
//      SDL_RenderCopy(renderer_, m_imgTiles, &dstrect2, &dstrect);
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

struct Main
{
    SDL_Window *window_; // SDL_Surface *gSurface;
    SDL_Renderer* renderer_;

    SDL_Texture* texture_;
    std::array<std::array<SDL_Texture*,9>,4> textures_;

    Mahjong m_;
    struct Test;

    ~Main();
    Main(int argc, char* const argv[]);
    int loop(int argc, char* const argv[]);

    void draw();

    void drawTilesRow(SDL_Texture* tils, int w, int h, int y)
    {
        int w0, h0;
        SDL_QueryTexture(tils, 0, 0, &w0, &h0);

        SDL_Rect srcRect = {0,y,w,h};
        while (srcRect.x + w < w0) {
            SDL_RenderCopy(renderer_, tils, &srcRect, &srcRect);
            srcRect.x += w;
        }
    }

    void drawTiles(SDL_Texture* tils)
    {
        drawTilesRow(tils, TILE_WIDTH, TILE_HEIGHT_COMPUTER, 0);
        //SDL_UpdateTexture(texture_, NULL, gSurface->pixels, gSurface->pitch);
        //SDL_RenderCopy(renderer_, texture_, NULL, NULL);
    }

    SDL_Texture *CreateTextureFromImageFile(const char *filename)
    {
        SDL_Surface *pic = IMG_Load(filename);
        ENSURE(pic, "IMG_Load: %s", SDL_GetError());
        //std::unique_ptr<SDL_Surface,decltype(& SDL_FreeSurface)> xfree(pic, SDL_FreeSurface);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer_, pic);
        SDL_FreeSurface(pic);
        return tex;
    }
};
struct Main::Test
{
    Main* thiz;
    Test(Main* p) : thiz(p) {}

    void shuffle() {
        std::vector<int> tils; //(4*9*3 + 4*7);
        for (int y=0; y<3; ++y) {
            for (int x=0; x<9; ++x) {
                for (int i=0; i<4; ++i) {
                    tils.push_back( (y<<4) | x );
                }
            }
        }
        for (int x=0; x<7; ++x) {
            for (int i=0; i<4; ++i) {
                tils.push_back( (3<<4) | x );
            }
        }
        std::random_shuffle(tils.begin(), tils.end());

        Mahjong::Init init(&thiz->m_);
        for (int i=0; i<4; ++i) {
            init.wall(std::rand()%4 ,std::vector<int>(tils.begin(), tils.begin() + (tils.size()/4)*i));
        }
    }
};

Main::~Main()
{
    SDL_Quit();
}
Main::Main(int argc, char* const argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0) { 
        ERR_EXIT("Unable to init SDL: %s", SDL_GetError());
    }
    window_ = SDL_CreateWindow("Mahjong"
            , SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 0, 0
            , SDL_WINDOW_FULLSCREEN_DESKTOP/*SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL*/);
    ENSURE(window_, "SDL_CreateWindow: %s", SDL_GetError());
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_PRESENTVSYNC);
    ENSURE(renderer_, "SDL_CreateRenderer: %s", SDL_GetError());

    char const* img_dir_ = "images/tiles";
    char const* imgs[4][9] = {
        {"character_1.png","character_2.png","character_3.png","character_4.png","character_5.png","character_6.png","character_7.png","character_8.png","character_9.png"},
        {"ball_1.png","ball_2.png","ball_3.png","ball_4.png","ball_5.png","ball_6.png","ball_7.png","ball_8.png","ball_9.png" },
        {"bamboo_1.png","bamboo_2.png","bamboo_3.png","bamboo_4.png","bamboo_5.png","bamboo_6.png","bamboo_7.png","bamboo_8.png","bamboo_9.png"},
        {"wind_east.png","wind_south.png","wind_west.png","wind_north.png","dragon_red.png","dragon_green.png","dragon_white.png", nullptr}
    };
    //{"flower_bamboo.png","flower_chrysanthemum.png","flower_orchid.png","flower_plum.png","season_fall.png","season_spring.png","season_summer.png","season_winter.png"};
    for (int x=0; x<4; ++x) {
        for (int y=0; y<9; ++y) {
            if (!imgs[x][y])
                break;
            char fn[512];
            snprintf(fn,sizeof(fn), "%s/%s", img_dir_, imgs[x][y]);
            textures_[x][y] = CreateTextureFromImageFile(fn);
        }
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(renderer_, 1024, 768);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 1024, 768);
    //texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 640, 480);
}

void Main::draw()
{
    auto& w = m_.wall_;
    if (w.empty())
        return;
    SDL_Rect dstRect = {};
    SDL_QueryTexture(textures_[0][0], 0, 0, &dstRect.w, &dstRect.h);

    int n = w.size()/4/2;
    for (int j=0; j<2; ++j) {
        dstRect.y = (dstRect.h+1)*j;
        for (int i=0; i < n; ++i) {
            int code = w[n*j+i];
            int y = (code >> 4) & 0x0f;
            int x = (code) & 0x0f;
            dstRect.x = (dstRect.w+1) * i;
            SDL_RenderCopy(renderer_, textures_[y][x], 0, &dstRect);
        }
    }
}
//gSurface = SDL_CreateRGBSurface(0, 640, 480, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
//SDL_FillRect(gSurface, &{64,48,64,48}, 0xff);

int Main::loop(int argc, char* const argv[])
{
    Test test(this); //ENSURE(argc>1, "argc"); SDL_Texture* tils = CreateTextureFromImageFile(argv[1]);

    while (1) {
        draw();
        SDL_RenderPresent(renderer_);
        SDL_Delay(10); 

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYUP:
                    switch (int sym = event.key.keysym.sym) {
                        case SDLK_ESCAPE: // If escape is pressed, return (and thus, quit)
                            return 0;
                        case SDLK_1: // If escape is pressed, return (and thus, quit)
                            test.shuffle();
                            break;
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

int main(int argc, char *argv[])
{
    Main app(argc, argv);
    app.loop(argc, argv);
    return 0;
}

