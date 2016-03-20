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
//#include "plog/Log.h"
//#include "plog/Appenders/ColorConsoleAppender.h"

#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...); //fflush(stderr);
}
#define ENSURE(c, ...) if(!(c))ERR_EXIT(__VA_ARGS__)

inline SDL_Point center_point(const SDL_Rect& a) { return SDL_Point{a.x+a.w/2, a.y+a.h/2}; }

inline std::array<int,2> center_row(int height, int rh) {
    return { height/2+1 - rh/2, height/2 + rh/2 };
}
inline int last_y(int height, int rh) { return (height/rh)*(rh); }
inline int nth_y(int nth, int height, int rh) { return (/*height/rh*/ + nth)*(rh); }
//inline std::array<int,2> last_row(int height, int rh) {
//    int x = height/2 + rh/2;
//    int n = (height-x) / (rh);
//    x += n*(rh);
//    return { x, x + rh };
//}
inline std::array<int,2> first_row(int height, int rh) {
    int x = height/2+1 - rh/2;
    int n = x / (rh);
    x -= n*(rh);
    return { x - rh, x };
}
void PrintEvent(const SDL_Event * event)
{
    switch (event->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
            SDL_Log("Window %d shown", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            SDL_Log("Window %d hidden", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            SDL_Log("Window %d exposed", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MOVED:
            SDL_Log("Window %d moved to %d,%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            SDL_Log("Window %d resized to %dx%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_Log("Window %d size changed to %dx%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            SDL_Log("Window %d minimized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            SDL_Log("Window %d maximized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_RESTORED:
            SDL_Log("Window %d restored", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_ENTER:
            SDL_Log("Mouse entered window %d",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            SDL_Log("Mouse left window %d", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            SDL_Log("Window %d gained keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            SDL_Log("Window %d lost keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_CLOSE:
            SDL_Log("Window %d closed", event->window.windowID);
            break;
        default:
            SDL_Log("Window %d got unknown event %d",
                    event->window.windowID, event->window.event);
            break;
    }
}
void PrintSize(SDL_Window* win, SDL_Renderer* ren, SDL_Texture* tex)
{
    SDL_DisplayMode dm; //SDL_GetNumVideoDisplays()
    SDL_GetCurrentDisplayMode(0, &dm);
    SDL_Log("\tDisplay %dx%d", dm.w, dm.h);
    int w,h;
    SDL_GetWindowSize(win, &w, &h);
    SDL_Log("\tWindow %dx%d", w, h);
    SDL_GetRendererOutputSize(ren, &w, &h);
    SDL_Log("\tRenderer %dx%d", w, h);
    //SDL_Rect rvp; SDL_RendererGetViewport(ren, &rvp); SDL_Log("\tViewport %dx%d+%d+%d", rvp.w, rvp.h, rvp.x, rvp.y);
    SDL_QueryTexture(tex, 0, 0, &w, &h);
    SDL_Log("\tTexture %dx%d", w,h);
}

// 万 筒 索 字
// 东 南 西 北 中 发 白
// 顺 刻 将
struct Hand : std::array<std::array<int8_t,9>,4>
{
    // std::array<int8_t,4> nc;

    Hand() : std::array<std::array<int8_t,9>,4>({}) {}
    //int8_t pair_idx = -1;
    //static char* parse(std::array<int8_t,9>& mj, char const* beg, char const* end);
    //static bool parse_file(Hand& mj, FILE* fp);

    std::array<int8_t,4> sums() {
        std::array<int8_t,4> ret = {};
        for (unsigned i=0; i < this->size(); ++i) {
            ret[i] = std::accumulate((*this)[i].begin(), (*this)[i].end(), 0);
        }
        return ret;
    }
    template<typename I> int copy(I it) const {
        int nc = 0;
        for (unsigned k=0; k < this->size(); ++k) {
            auto& a = (*this)[k];
            for (unsigned j=0; j < a.size(); ++j) {
                nc += a[j];
                for (int i = 0; i < a[j]; ++i) {
                    *it = 9*k + j;
                    ++it;
                }
            }
        }
        return nc;
    }
};

struct Mahjong : std::array<Hand,4>
{
    enum { Nplayer = 4, Ntiles=(9*3+7)*4 }; //enum { East = 0, South, West, North };
    //int8_t wall_[Ntiles]; short end_, beg_;
    std::deque<int> wall_;
    std::vector<int> discard_;
    int8_t first_hand_pos_ = -1;
    struct Init;

    Hand& at(int pos) const { return const_cast<Hand&>( (*this)[pos%Nplayer] ); }

    void deal(int pos, int nc=13);
    void prepare(int pos) {
        //Hand& h = at(pos); //(*this)[pos%Nplayer];
        //std::array<int8_t,4> sums = hand.sums();
        //int nc = std::accumulate(sums.begin(), sums.end(), 0);
    }

    void fetch(int pos);
    void discard(int pos);
    void hu(int pos);
    void pen(int pos);
    void gan(int pos);
    void hint(int pos);
    void robot(int pos);
};

struct Mahjong::Init : std::array<std::vector<int>,4>
{
    Mahjong* thiz;
    int n_ = 0;
    Init(Mahjong* p) : thiz(p) {}

    int wall(int x, std::vector<int> v) {
        ENSURE(x<4, "wall");
        (*this)[x] = std::move(v);
        if (++n_ == 4) {
            for (int x=0; x<4; ++x) {
                auto& v = (*this)[x];
                thiz->wall_.insert(thiz->wall_.end(), v.begin(), v.end());
            }
        }
        return (4-n_);
    }
};

void Mahjong::deal(int pos, int nc)
{
    first_hand_pos_ = pos;
    nc *= Nplayer;
    while (nc > Nplayer) {
        for (int n=0; n < 4; ++n)
            fetch(pos);
        nc -= 4;
        ++pos;
    }
    ENSURE(pos%Nplayer==first_hand_pos_, "deal pos");
    while (nc > 0) {
        fetch(pos);
        prepare(pos);
        ++pos;
        nc -= 1;
    }
    ENSURE(pos%Nplayer==first_hand_pos_, "deal pos");
}

void Mahjong::fetch(int pos)
{
    Hand& h = at(pos);
    int cx = wall_.front() & 0x3f; wall_.pop_front();
    int w = cx/9;
    int v = cx%9;
    h[w][v]++;
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
    SDL_Texture* facing_[4];
    std::vector<SDL_Texture*> tile_textures_; //std::array<std::array<SDL_Texture*,9>,4> tile_textures_; // = {};

    Mahjong m_;
    SDL_Rect squa_ = {}, rect_ = {}, rect_hv_;
    short tiles_total_ = 0;
    struct Grid;
    struct UInput;

    ~Main();
    Main(int argc, char* const argv[]);
    int loop(int argc, char* const argv[]);

    void draw();
    void draw_rotate_test();
    void draw_discard();
    void draw_background();
    void draw_hands();
    void draw_wall();

    void draw_hand_tiles(SDL_Texture* tex, std::vector<int>& tils);

    SDL_Texture *create_texture(const char *filename) {
        SDL_Surface *pic = IMG_Load(filename);
        ENSURE(pic, "IMG_Load: %s", SDL_GetError());
        //std::unique_ptr<SDL_Surface,decltype(& SDL_FreeSurface)> xfree(pic, SDL_FreeSurface);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer_, pic);
        SDL_FreeSurface(pic);
        return tex;
    }

    void init1() {
        tiles_total_ = m_.wall_.size();
        int nor = tiles_total_/4/2;
        int w, h;
        SDL_QueryTexture(tile_textures_[0], 0, 0, &w, &h);
        float ratio = float(squa_.w)/((w+1)*nor+(h+1)*6+w*2);
        rect_.w = w*ratio;
        rect_.h = h*ratio;
    }
    SDL_Texture* tiletex(int cx) const {
        return tile_textures_[cx & 0x3f];
        //cx &= 0x3f;
        //int w = cx/9; //int v = cx%9;
        //return this->tile_textures_[w][v];
    };
};

struct Main::Grid
{
    int w_,h_;
};


struct Main::UInput
{
    Main* thiz;
    UInput(Main* p) : thiz(p) {}

    int pressed(int ksym) {
        auto& m = thiz->m_;
        if (SDLK_0 == ksym) {
            m = Mahjong{};
            shuffle();
        } else if (thiz->rect_.w > 0) switch (ksym) {
            case SDLK_1: // If escape is pressed, return (and thus, quit)
                m.deal(rand()%4);
                break;
            default: ;
        }
        return ksym;
    }

    void shuffle() {
        std::vector<int> tils; //(4*9*3 + 4*7);
        for (int x=0, n=thiz->tile_textures_.size(); x<n; ++x) {
            for (int y=0; y<4; ++y)
                tils.push_back( (y<<6) | x );
        }

        //=std::random_shuffle(tils.begin(), tils.end());
        std::vector<int> ridx = {3,2,1,0};
        //=std::random_shuffle(ridx.begin(), ridx.end());
        //LOG_DEBUG << tils.size() <<' '<< tils.size()/4;

        Mahjong::Init init(&thiz->m_);
        for (int n=tils.size()/4, i=0; i<4; ++i) {
            auto it = tils.begin() + n*i;
            if (init.wall(ridx.back(), std::vector<int>(it, it+n)) == 0)
                thiz->init1();
            ridx.pop_back();
        }
    }
};

Main::~Main()
{
    SDL_Quit();
}

Main::Main(int argc, char* const argv[]) //: tile_textures_({})
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0) { 
        ERR_EXIT("Unable to init SDL: %s", SDL_GetError());
    }

    // 万 筒 索 字
    // 东 南 西 北 中 发 白
    // 顺 刻 将
    char const* img_tiles[] = {
        "character_1.png","character_2.png","character_3.png","character_4.png","character_5.png","character_6.png","character_7.png","character_8.png","character_9.png",
        "ball_1.png","ball_2.png","ball_3.png","ball_4.png","ball_5.png","ball_6.png","ball_7.png","ball_8.png","ball_9.png" ,
        "bamboo_1.png","bamboo_2.png","bamboo_3.png","bamboo_4.png","bamboo_5.png","bamboo_6.png","bamboo_7.png","bamboo_8.png","bamboo_9.png",
        "wind_east.png","wind_south.png","wind_west.png","wind_north.png","dragon_red.png","dragon_green.png","dragon_white.png"
    };
    //{"flower_bamboo.png","flower_chrysanthemum.png","flower_orchid.png","flower_plum.png","season_fall.png","season_spring.png","season_summer.png","season_winter.png"};
    char const* img_dir = "images/tiles";

    std::vector<SDL_Surface*> surfaces;
    for (char const* name : img_tiles) {
        char fn[512];
        snprintf(fn,sizeof(fn), "%s/%s", img_dir, name);
        surfaces.push_back( IMG_Load(fn) );
    }

    SDL_DisplayMode dm; //SDL_GetNumVideoDisplays()
    SDL_GetCurrentDisplayMode(0, &dm); //SDL_GetRendererOutputSize(renderer_, &w, &h);SDL_GetWindowSize(window_, &w, &h);SDL_RendererGetViewport(renderer_, &rect);
    squa_.w = squa_.h = std::min(dm.w,dm.h);
    squa_.x = squa_.y = 0;
    window_ = SDL_CreateWindow("Mahjong", SDL_WINDOWPOS_UNDEFINED,0, squa_.w,squa_.h, SDL_WINDOW_RESIZABLE); //SDL_WINDOW_RESIZABLE SDL_WINDOW_BORDERLESS,SDL_WINDOW_FULLSCREEN_DESKTOP,SDL_WINDOW_FULLSCREEN

    ENSURE(window_, "SDL_CreateWindow: %s", SDL_GetError());
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_PRESENTVSYNC);
    ENSURE(renderer_, "SDL_CreateRenderer: %s", SDL_GetError());
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    //SDL_RenderSetLogicalSize(renderer_, 1024, 768);

    //SDL_TEXTUREACCESS_STREAMING SDL_TEXTUREACCESS_STATIC
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, squa_.w, squa_.h);
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    //for (int i=0; i<4; ++i)
    //    facing_[i] = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, squa_.w, squa_.h/2);

    tile_textures_.reserve(surfaces.size());
    for (auto ptr : surfaces) {
        tile_textures_.push_back( SDL_CreateTextureFromSurface(renderer_, ptr) );
        SDL_FreeSurface(ptr);
    }

    PrintSize(window_, renderer_, texture_);
}

void Main::draw_discard()
{
}

auto renderTarget0 = [](SDL_Renderer* r){ SDL_SetRenderTarget(r,NULL); };

void Main::draw_hands()
{
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    for (unsigned i=0; i<m_.size(); ++i) {
        std::vector<int> tils;
        m_[i].copy( std::back_inserter(tils) );

        SDL_RenderClear(renderer_);
        draw_hand_tiles(texture_, tils);

        SDL_SetRenderTarget(renderer_, 0);
        //SDL_RenderCopy(renderer_, texture_, 0, &squa_);
        SDL_RenderCopyEx(renderer_, texture_, 0, &squa_,   90*i, 0, SDL_FLIP_NONE);
        SDL_SetRenderTarget(renderer_, texture_);
    }
    SDL_SetRenderTarget(renderer_, 0);
}

void Main::draw_hand_tiles(SDL_Texture* tex, std::vector<int>& tils)
{
    SDL_Rect rect = {};
    SDL_GetRendererOutputSize(renderer_, &rect.w, &rect.h);
    SDL_Point pc = center_point(rect);

    SDL_Rect dr = rect_;
    int step = rect_.w+1;
    dr.y = pc.y + last_y(rect.h/2, step) - rect_.w;
    dr.x = pc.x - nth_y(tils.size()/2, rect.w/2, step) +1;
    rect_hv_.x = dr.x;
    rect_hv_.y = dr.y;
    rect_hv_.w = (rect_.w+1) * tils.size();
    rect_hv_.h = rect_.h;
    for (int cx : tils) {
        SDL_RenderCopy(renderer_, tiletex(cx), 0, &dr);
        dr.x += step;
    }
}

void Main::draw_wall()
{
    int fr0 = 0;
    int nor = tiles_total_/4/2;
    SDL_Point pc = center_point(squa_);
    SDL_Rect dr = rect_;

    dr.x = pc.x - nor/2*(rect_.w+1) +1;
    dr.y = pc.y +1;
    fr0 = 0;
    for (int i=0; i<nor; ++i){
        SDL_RenderCopy(renderer_, tiletex(m_.wall_[fr0+i]), 0, &dr);
        dr.x += 1+rect_.w;
    }
    dr.x = pc.x - nor/2*(rect_.w+1) +1;
    dr.y = pc.y +1 + rect_.h + 1;
    fr0 += nor;
    for (int i=0; i<nor; ++i) {
        SDL_RenderCopy(renderer_, tiletex(m_.wall_[fr0+i]), 0, &dr);
        dr.x += 1+rect_.w;
    }
}
//gSurface = SDL_CreateRGBSurface(0, 640, 480, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
//SDL_FillRect(gSurface, &{64,48,64,48}, 0xff);

void Main::draw_background()
{
    SDL_SetRenderDrawColor(renderer_, 5, 5, 5, 255);
    SDL_RenderClear(renderer_);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer_, &squa_);

    //std::array<int,2> px = center_row(squa_.w, rect_.w +1);
    //std::array<int,2> py = center_row(squa_.h, rect_.w +1);
    ////std::array<int,2> py = first_row<1>(squa_.h, rect_.w);
    ////int y = last_row<1>(squa_.h, rect_.w)[1] - rect_.h;
    //SDL_RenderDrawLine(renderer_, px[0], 0, px[0], squa_.h);
    //SDL_RenderDrawLine(renderer_, px[1], 0, px[1], squa_.h);
    //SDL_RenderDrawLine(renderer_, py[0], 0, py[0], squa_.w);
    //SDL_RenderDrawLine(renderer_, py[1], 0, py[1], squa_.w);

    SDL_Point pc = center_point(squa_);

    SDL_SetRenderDrawColor(renderer_, 16, 16, 16, 255);
    SDL_RenderDrawLine(renderer_, pc.x, squa_.y, pc.x, squa_.y+squa_.h);
    SDL_RenderDrawLine(renderer_, squa_.x, pc.y, squa_.x+squa_.w, pc.y);

    SDL_SetRenderDrawColor(renderer_, 10, 10, 10, 255);

    int s = rect_.w+1;
    while (pc.y - s > squa_.y) {
        SDL_RenderDrawLine(renderer_, squa_.x, pc.y-s, squa_.x+squa_.w, pc.y-s);
        SDL_RenderDrawLine(renderer_, squa_.x, pc.y+s, squa_.x+squa_.w, pc.y+s);
        SDL_RenderDrawLine(renderer_, pc.x-s, squa_.y, pc.x-s, squa_.y+squa_.h);
        SDL_RenderDrawLine(renderer_, pc.x+s, squa_.y, pc.x+s, squa_.y+squa_.h);
        s += rect_.w+1;
    }
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
}

void Main::draw()
{
    if (rect_.w < 1)
        return;

    draw_background();

    draw_wall();

    draw_hands();

    draw_discard();
    //draw_rotate_test();
}

void Main::draw_rotate_test()
{
    SDL_Point pc = center_point(squa_);
    SDL_Rect dr = rect_;
    dr.x = pc.x + rect_.w;
    dr.y = pc.y + rect_.w;
    SDL_RenderCopy(renderer_, tile_textures_[0], 0, &dr);
    dr.x = pc.x + rect_.w*3;
    dr.y = pc.y + rect_.w*3;
    SDL_RenderCopyEx(renderer_, tile_textures_[0], 0, &dr, 90, 0, SDL_FLIP_NONE);
    dr.x = pc.x + rect_.w*6;
    dr.y = pc.y + rect_.w*6;
    SDL_Point cr = {}; //{dr.w/2, dr.h/2};
    SDL_RenderCopyEx(renderer_, tile_textures_[0], 0, &dr, 90, &cr, SDL_FLIP_NONE);
}

int Main::loop(int argc, char* const argv[])
{
    UInput input(this); //ENSURE(argc>1, "argc"); SDL_Texture* tils = create_texture(argv[1]);
    SDL_Event event;

    while (1) {
        draw();
        SDL_RenderPresent(renderer_);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    return(0);
                case SDL_KEYUP:
                    switch (int sym = event.key.keysym.sym) {
                        case SDLK_ESCAPE: // If escape is pressed, return (and thus, quit)
                            return 0;
                        default: input.pressed(sym);//;drawrect(gSurface, {64,48,64,48}, 0x88ff|((sym*3)<<16)&0xffffff);
                    }
                    break;
                case SDL_WINDOWEVENT:
                    PrintEvent(&event);
                    PrintSize(window_, renderer_, texture_);
                    break;
            }
        }

        SDL_Delay(10); 
    }
    return 0;
}

int main(int argc, char *argv[])
{
    //static plog::ColorConsoleAppender<plog::TxtFormatter> loga;
    //plog::init(plog::verbose, &loga);
    Main app(argc, argv);
    app.loop(argc, argv);
    return 0;
}

