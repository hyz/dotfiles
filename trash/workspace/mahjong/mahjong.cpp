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
#define ensure(c, ...) if(!(c))ERR_EXIT(__VA_ARGS__)

// 万 筒 索 字
// 东 南 西 北 中 发 白
// 顺 刻 将
struct Hand : std::array<std::array<int8_t,9>,4>
{
    // std::array<int8_t,4> nc;
    //int8_t pair_idx = -1;

    Hand() : std::array<std::array<int8_t,9>,4>({}) {}
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
    struct Tiles : std::vector<int8_t> {
        short last=0, first=0;
        int8_t pop(bool tail) {
            return (*this)[tail ? last-- : first++];
        }
        int size() const { return int(std::vector<int8_t>::size()); }
        bool empty() const { return first > (last+int(size()))%size();}
    };
    Tiles wall_; //int8_t wall_[Ntiles]; short end_, beg_; //std::deque<int> wall_;
    std::vector<int8_t> discard_;
    int8_t first_hand_pos_ = -1;
    struct Init;

    Hand& at(int pos) const { return const_cast<Hand&>( (*this)[pos%Nplayer] ); }

    void deal(int pos, int beg, int to_pos, int noh=13);
    void prepare(int pos) {
        //Hand& h = at(pos); //(*this)[pos%Nplayer];
        //std::array<int8_t,4> sums = hand.sums();
        //int noh = std::accumulate(sums.begin(), sums.end(), 0);
    }

    void fetch(int pos, bool tail=0);
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
        ensure(x<4, "wall");
        (*this)[x] = std::move(v);
        if (++n_ == 4) {
            thiz->wall_.reserve(v.size()*4);
            for (int x=0; x<4; ++x) {
                for (int cx : (*this)[x])
                    thiz->wall_.push_back(cx & 0x3f);
                //thiz->wall_.insert(thiz->wall_.end(), v.begin(), v.end());
            }
        }
        return (4-n_);
    }
};

void Mahjong::deal(int pos, int beg, int to_pos, int noh)
{
    first_hand_pos_ = pos;
    noh *= Nplayer;
    while (noh > Nplayer) {
        for (int n=0; n < 4; ++n)
            fetch(pos);
        noh -= 4;
        ++pos;
    }
    ensure(pos%Nplayer==first_hand_pos_, "deal pos");
    while (noh > 0) {
        fetch(pos);
        prepare(pos);
        ++pos;
        noh -= 1;
    }
    ensure(pos%Nplayer==first_hand_pos_, "deal pos");
}

void Mahjong::fetch(int pos, bool tail)
{
    ensure(!wall_.empty(), "fetch wall:empty");
    Hand& h = at(pos);
    int cx = wall_.pop(tail); //wall_.front() & 0x3f; wall_.pop_front();
    int w = cx/9;
    int v = cx%9;
    h[w][v]++;
}

struct View_resource
{
    SDL_Window *window; // SDL_Surface *gSurface;
    SDL_DisplayMode displayMode;
    SDL_Renderer* renderer;
    std::vector<SDL_Texture*> tiles; //std::array<std::array<SDL_Texture*,9>,4> tiles; // = {};

    View_resource();
    ~View_resource();
};

View_resource::~View_resource()
{
    for (SDL_Texture* ptr : tiles)
        SDL_DestroyTexture(ptr);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

View_resource::View_resource()
{
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE);// >=0, "SDL_Init: %s", SDL_GetError()
    SDL_DisplayMode dm; //SDL_GetNumVideoDisplays()
    SDL_GetCurrentDisplayMode(0, &dm); //SDL_GetRendererOutputSize(renderer, &w, &h);SDL_GetWindowSize(window, &w, &h);SDL_RendererGetViewport(renderer, &rect);
    int wh = std::min(dm.w,dm.h);

    //window = SDL_CreateWindow("Mahjong", SDL_WINDOWPOS_UNDEFINED,0, wh,wh, SDL_WINDOW_RESIZABLE); //SDL_WINDOW_BORDERLESS,SDL_WINDOW_FULLSCREEN_DESKTOP,SDL_WINDOW_FULLSCREEN
    //window = SDL_CreateWindow("Mahjong", SDL_WINDOWPOS_UNDEFINED,0, wh,wh, 0); //SDL_WINDOW_FULLSCREEN_DESKTOP SDL_WINDOW_BORDERLESS,,SDL_WINDOW_FULLSCREEN
    window = SDL_CreateWindow("Mahjong", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, wh,wh, SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP); //SDL_WINDOW_RESIZABLE SDL_WINDOW_BORDERLESS,SDL_WINDOW_FULLSCREEN_DESKTOP,SDL_WINDOW_FULLSCREEN
    SDL_GetWindowDisplayMode(window, &displayMode);
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE);
    //renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);//, "SDL_CreateRenderer: %s", SDL_GetError()
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    //SDL_RenderSetLogicalSize(renderer, 1024, 768);

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

    int nt = sizeof(img_tiles)/sizeof(img_tiles[0]);
    ensure(nt%2==0, "N-tiles %d", nt);

    tiles.reserve(nt);
    for (char const* name : img_tiles) {
        char fn[512];
        snprintf(fn,sizeof(fn), "%s/%s", img_dir, name);
        SDL_Surface* surf = IMG_Load(fn);
        ensure(surf, "IMG_Load %s: %s", fn, SDL_GetError());
        tiles.push_back( SDL_CreateTextureFromSurface(renderer, surf) );
        SDL_FreeSurface(surf);
    }
    SDL_Log("tiles.size=%d %d", (int)tiles.size(), nt);
}

struct Layout_size
{
    int w, h;  // tile width,length
    int n, sp;     // wall-length, space
    int s; // texture-size
    int x, y;  // center-point of world
    Layout_size(SDL_Renderer* renderer, SDL_Texture* tile, int nts) {
        int w0,h0;
        SDL_GetRendererOutputSize(renderer, &w0, &h0); //SDL_GetCurrentDisplayMode SDL_GetRendererOutputSize SDL_GetWindowSize
        int w1,h1;
        SDL_QueryTexture(tile, 0, 0, &w1, &h1); //wh_ratio = double(w1)/h1;
        s = std::min(w0,h0);
        n = nts/4/2;
        //--------------------------------------
        // h/w = h1/w1;
        // s - 2*(h+w) = ?h + sp + n*w
        // sp >= w/2;
        //--------------------------------------
        w = s/(double(4*h1)/w1 + double(5+2*n)/2);
        h = double(h1)/w1*w;
        sp = s - 2*(h+w) - 2*h - n*w;
        x = w0/2; y = h0/2;
    }
};

struct Renderer
{
    View_resource* res;
    SDL_Texture* texture_;
    Layout_size size_; //int nline_; int w_, h_; int wh_;

    Renderer(View_resource& vr, int ncol);
    ~Renderer() {
        SDL_Log("DestroyTexture(texture_)");
        SDL_DestroyTexture(texture_);
    }

    //SDL_Point center_point(int x=0,int y=0) { return SDL_Point{x+w_*nline_, y+w_*nline_}; }
    //SDL_Rect main_rect(int x=0,int y=0) { return SDL_Rect{x,y, x+w_*nline_*2, y+w_*nline_*2}; }

    void draw_background();
};

struct View : Renderer
{
    View(View_resource& vr);

    SDL_Texture* tiletex(int cx) const { return res->tiles[cx & 0x3f]; };

    void draw(Mahjong const&);

    void draw_test(SDL_Texture* tile);
    void draw_discard();
    void draw_hands();
    void draw_wall(Mahjong::Tiles const& wall, int pos0);

    void draw_hand_tiles(SDL_Texture* tex, std::vector<int>& tils);

    //static int Ncol(SDL_Texture* tile, int nts) {
    //    nts /= (4*2);
    //    int w,h;
    //    SDL_QueryTexture(tile, 0, 0, &w, &h);
    //    return ((nts+1)/2 + 1 + (3*h+(w-2))/w) *2;
    //}
};

Renderer::Renderer(View_resource& vr, int nts)
    : res(&vr)
    , size_(vr.renderer, vr.tiles[0], nts)
{
    //nline_ = (1+3+(nts/4/2-2+1)+3+1) / 2; // ((nts+1)/2 + 1 + (3*h+(w-2))/w) *2;
    //ensure(ncol%2==0, "Ncol %d", ncol);

    // SDL_SetRenderTarget(res->renderer, 0);
    // SDL_SetRenderDrawBlendMode(res->renderer, SDL_BLENDMODE_BLEND);
    //int w0,h0;
    //SDL_GetRendererOutputSize(res->renderer, &w0, &h0); //SDL_GetCurrentDisplayMode SDL_GetRendererOutputSize SDL_GetWindowSize
    //int w1,h1;
    //SDL_QueryTexture(vr.tiles[0], 0, 0, &w1, &h1); //wh_ratio = double(w1)/h1;
    //wh_ = std::min(w0,h0);
    //nts=nts/4/2;

    //double ratio = double(h1)/w1;
    //w_ = (wh_-sx)/(nts+2+3*ratio); //nts*w + h + 2*h + 2*w = wh_ - sx;// <=wh_ sx>0; (nts+2+3*(double(h1)/w1))*w = wh_-sx;// <=wh_ sx>0;
    //h_ = w*ratio;
    //n = (wh_ - 2*h_) / w;
    //sp_ = (wh_ - 2*h_) / w;

    //int w = wh_/2 / (nline_-1 + double(h1)/w1);
    //h_ = double(w)/w1 * h1 -1;
    //w_ = w -1;

    SDL_Log("CreateTexture %d", size_.s);
    texture_ = SDL_CreateTexture(res->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size_.s,size_.s); //SDL_TEXTUREACCESS_STREAMING SDL_TEXTUREACCESS_STATIC
    //texture_ = SDL_CreateTexture(res->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, size_.s,size_.s);
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
}

View::View(View_resource& vr)
    : Renderer(vr, 4*vr.tiles.size())
{
}

struct Main : View_resource
{
    Mahjong m_;
    View view_;
    struct UInput;

    enum { STAGE_0, STAGE_1 };
    int8_t stage_ = 0; //short tiles_total_ = 0;

    Main(int argc, char* const argv[]);
    ~Main();
    int loop(int argc, char* const argv[]);

    void stage(int s) { stage_ = s; }
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
        } else /*if (thiz->rect_.w > 0)*/ switch (ksym) {
            case SDLK_1: // If escape is pressed, return (and thus, quit)
                m.deal(rand()%4, rand()%thiz->view_.size_.n, rand()%4);
                break;
            default: ;
        }
        return ksym;
    }

    void shuffle() {
        std::vector<int> tils; //(4*9*3 + 4*7);
        for (int x=0, n=thiz->tiles.size(); x<n; ++x) {
            for (int y=0; y<4; ++y)
                tils.push_back( (y<<6) | x );
        }

        //==std::random_shuffle(tils.begin(), tils.end());
        std::vector<int> ridx = {3,2,1,0};
        //==std::random_shuffle(ridx.begin(), ridx.end());
        //LOG_DEBUG << tils.size() <<' '<< tils.size()/4;

        Mahjong::Init init(&thiz->m_);
        for (int n=tils.size()/4, i=0; i<4; ++i) {
            auto it = tils.begin() + n*i;
            if (init.wall(ridx.back(), std::vector<int>(it, it+n)) == 0)
                thiz->stage(Main::STAGE_1); //thiz->view_.init1();
            ridx.pop_back();
        }
    }
};

Main::~Main()
{
}

Main::Main(int argc, char* const argv[])
    : view_(*this)
{}

void Renderer::draw_background()
{
    SDL_SetRenderDrawColor(res->renderer, 0,0,0, 255);
    SDL_RenderClear(res->renderer);

    int xp[2], yp[2];
    xp[0] = size_.x - size_.s/2;// + size_.h + size_.w;
    xp[1] = size_.x + size_.s/2;// - size_.h - size_.w;
    yp[0] = size_.y - size_.s/2;// + size_.h + size_.w;
    yp[1] = size_.y + size_.s/2;// - size_.h - size_.w;

    SDL_SetRenderDrawColor(res->renderer, 12,12,12, 255);

    SDL_RenderDrawLine(res->renderer, size_.x, yp[0], size_.x, yp[1]);
    SDL_RenderDrawLine(res->renderer, xp[0], size_.y, xp[1], size_.y);
    SDL_RenderDrawLine(res->renderer, xp[0], yp[0], xp[1], yp[1]);
    SDL_RenderDrawLine(res->renderer, xp[0], yp[1], xp[1], yp[0]);

    SDL_RenderDrawLine(res->renderer, xp[0]+size_.h, yp[0], xp[0]+size_.h, yp[1]);
    SDL_RenderDrawLine(res->renderer, xp[0]+size_.h+size_.w, yp[0], xp[0]+size_.h+size_.w, yp[1]);
    SDL_RenderDrawLine(res->renderer, xp[1]-size_.h, yp[0], xp[1]-size_.h, yp[1]);
    SDL_RenderDrawLine(res->renderer, xp[1]-size_.h-size_.w, yp[0], xp[1]-size_.h-size_.w, yp[1]);
    SDL_RenderDrawLine(res->renderer, xp[0], yp[0]+size_.h, xp[1], yp[0]+size_.h);
    SDL_RenderDrawLine(res->renderer, xp[0], yp[0]+size_.h+size_.w, xp[1], yp[0]+size_.h+size_.w);
    SDL_RenderDrawLine(res->renderer, xp[0], yp[1]-size_.h, xp[1], yp[1]-size_.h);
    SDL_RenderDrawLine(res->renderer, xp[0], yp[1]-size_.h-size_.w, xp[1], yp[1]-size_.h-size_.w);

    xp[0] = size_.x - size_.s/2 + size_.h + size_.w;
    xp[1] = size_.x + size_.s/2 - size_.h - size_.w;
    yp[0] = size_.y - size_.s/2 + size_.h + size_.w;
    yp[1] = size_.y + size_.s/2 - size_.h - size_.w;
    SDL_RenderDrawLine(res->renderer, xp[0], yp[1]-size_.h*2, xp[1]-size_.h*2-size_.sp, yp[1]-size_.h*2);
    SDL_RenderDrawLine(res->renderer, xp[1]-size_.h*2-size_.sp, yp[1]-size_.h*2, xp[1]-size_.h*2-size_.sp, yp[1]);

    SDL_RenderDrawLine(res->renderer, xp[0]+size_.h*2+size_.sp, yp[0]+size_.h*2, xp[1], yp[0]+size_.h*2);
    SDL_RenderDrawLine(res->renderer, xp[0]+size_.h*2+size_.sp, yp[0]+size_.h*2, xp[0]+size_.h*2+size_.sp, yp[0]);

    SDL_RenderDrawLine(res->renderer, xp[0]+size_.h*2, yp[0], xp[0]+size_.h*2, yp[1]-size_.h*2-size_.sp);
    SDL_RenderDrawLine(res->renderer, xp[0], yp[1]-size_.h*2-size_.sp, xp[0]+size_.h*2, yp[1]-size_.h*2-size_.sp);

    SDL_RenderDrawLine(res->renderer, xp[1]-size_.h*2, yp[0]+size_.h*2+size_.sp, xp[1]-size_.h*2, yp[1]);
    SDL_RenderDrawLine(res->renderer, xp[1]-size_.h*2, yp[0]+size_.h*2+size_.sp, xp[1], yp[0]+size_.h*2+size_.sp);

    ////SDL_SetRenderTarget(res->renderer, 0);
    //int w0,h0;
    //SDL_GetRendererOutputSize(res->renderer, &w0, &h0); //SDL_GetCurrentDisplayMode SDL_GetRendererOutputSize SDL_GetWindowSize

    //SDL_SetRenderDrawColor(res->renderer, 0,0,0, 255);
    //SDL_RenderClear(res->renderer);

    ////SDL_Rect rect = main_rect(x_,y_);
    //int x=w0/2, y=h0/2; //center_point(x_,y_);
    //int w=w_+1;
    //int wh = wh_/2;//w*(nline_-1) + h_;

    //SDL_SetRenderDrawColor(res->renderer, 12,12,12, 255);
    //SDL_RenderDrawLine(res->renderer, x, y-wh, x, y+wh);
    //SDL_RenderDrawLine(res->renderer, x-wh, y, x+wh, y);

    //SDL_SetRenderDrawColor(res->renderer, 6,6,6, 255);
    //for (int i=1; i < nline_; ++i) {
    //    SDL_RenderDrawLine(res->renderer, x-w*i, y-wh, x-w*i, y+wh);
    //    SDL_RenderDrawLine(res->renderer, x+w*i, y-wh, x+w*i, y+wh);
    //    SDL_RenderDrawLine(res->renderer, x-wh, y-w*i, x+wh, y-w*i);
    //    SDL_RenderDrawLine(res->renderer, x-wh, y+w*i, x+wh, y+w*i);
    //}
    //SDL_SetRenderDrawColor(res->renderer, 0,0,0, 0);
}


void View::draw_discard()
{
}

//auto renderTarget0 = [](SDL_Renderer* r){ SDL_SetRenderTarget(r,NULL); };

//void View::draw_hands()
//{
//    SDL_SetRenderTarget(renderer, texture_);
//    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
//    for (unsigned i=0; i<m_.size(); ++i) {
//        std::vector<int> tils;
//        m_[i].copy( std::back_inserter(tils) );
//
//        SDL_RenderClear(renderer);
//        draw_hand_tiles(texture_, tils);
//
//        SDL_SetRenderTarget(renderer, 0);
//        //SDL_RenderCopy(renderer, texture_, 0, &squa_);
//        SDL_RenderCopyEx(renderer, texture_, 0, &squa_,   90*i, 0, SDL_FLIP_NONE);
//        SDL_SetRenderTarget(renderer, texture_);
//    }
//    SDL_SetRenderTarget(renderer, 0);
//}

//void View::draw_hand_tiles(SDL_Texture* tex, std::vector<int>& tils)
//{
//    SDL_Rect rect = {};
//    SDL_GetRendererOutputSize(renderer, &rect.w, &rect.h);
//    SDL_Point pc = center_point(rect);
//
//    SDL_Rect dr = rect_;
//    int step = rect_.w+1;
//    dr.y = pc.y + last_y(rect.h/2, step) - rect_.w;
//    dr.x = pc.x - nth_y(tils.size()/2, rect.w/2, step) +1;
//    rect_hv_.x = dr.x;
//    rect_hv_.y = dr.y;
//    rect_hv_.w = (rect_.w+1) * tils.size();
//    rect_hv_.h = rect_.h;
//    for (int cx : tils) {
//        SDL_RenderCopy(renderer, tiletex(cx), 0, &dr);
//        dr.x += step;
//    }
//}

void View::draw_wall(Mahjong::Tiles const& wall, int pos0)
{
    if (wall.empty()) //(wall.first >= wall.last + (int)wall.size())
        return;

    for (int beg = (size_.n*2)*pos0, end = beg + (size_.n*2)
            ; beg < end; ++beg) {
        ;
    }

    ////SDL_SetRenderTarget(res->renderer, 0);
    //int w0,h0;
    //SDL_GetRendererOutputSize(res->renderer, &w0, &h0);

    //int nt = wall.size();
    //int nr = nt/4/2;
    //int w = w_+1;
    //int y = wh_/2 + (nline_-2)*w - h_;
    //int x = wh_/2 - (nline_-3)*w + (nr-1)*w; //(nline_-6)*w;

    //int last = wall.last + nt;
    //for (int pos=pos0; pos < pos0+4; ++pos) {
    //    SDL_SetRenderTarget(res->renderer, texture_);
    //    //SDL_SetRenderDrawColor(res->renderer, 0,0,0, 0);
    //    //SDL_SetRenderDrawBlendMode(res->renderer, SDL_BLENDMODE_NONE);
    //    SDL_RenderClear(res->renderer);

    //    int beg = (pos%4) * nr;
    //    if (beg < wall.last)
    //        beg += nt;
    //    int end = beg + nr*2;
    //    for (int i=beg; i < end; ++i) {
    //        if (wall.first <= i && i <= last) {
    //            int j = i-beg;
    //            SDL_Rect dr = {x - w*(j/2), y - w*(j%2), w_,h_};
    //            SDL_Texture* tex = tiletex( wall[ pos0*nr*2 + j] );
    //            SDL_RenderCopy(res->renderer, tex, 0, &dr);
    //        }
    //    }

    //    SDL_SetRenderTarget(res->renderer, NULL);
    //    //SDL_SetRenderDrawColor(res->renderer, 0,0,0, 0);
    //    //SDL_SetRenderDrawBlendMode(res->renderer, SDL_BLENDMODE_BLEND);
    //    //SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    //    SDL_Rect rect = {w0/2-wh_/2,h0/2-wh_/2, wh_,wh_};
    //    SDL_RenderCopyEx(res->renderer, texture_, 0, &rect, (pos % 4)*90, 0, SDL_FLIP_NONE);
    //}
}
//gSurface = SDL_CreateRGBSurface(0, 640, 480, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
//SDL_FillRect(gSurface, &{64,48,64,48}, 0xff);

void View::draw(Mahjong const& m)
{
    draw_wall(m.wall_, 0);
    draw_wall(m.wall_, 1);
    draw_wall(m.wall_, 2);
    draw_wall(m.wall_, 3);

    //draw_hands();

    //draw_discard();
}

void View::draw_test(SDL_Texture* tile)
{
    //static SDL_Texture*texture = SDL_CreateTexture(res->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET , size_.s,size_.s); //SDL_TEXTUREACCESS_STREAMING SDL_TEXTUREACCESS_STATIC
    //texture = SDL_CreateTexture(res->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, size_.s,size_.s);
    static SDL_Texture*texture = 0;
    if (!texture) {
        texture = SDL_CreateTexture(res->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size_.s,size_.s); //SDL_TEXTUREACCESS_STREAMING SDL_TEXTUREACCESS_STATIC
        SDL_Log("size_.s %d test", size_.s);
    }
    texture = texture_;
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(res->renderer, 0,0,0, 0);

    SDL_SetRenderTarget(res->renderer, texture);
    //SDL_SetRenderDrawColor(res->renderer, 0,0,0, 0);
    //SDL_SetRenderDrawBlendMode(res->renderer, SDL_BLENDMODE_NONE);
    SDL_RenderClear(res->renderer);
    {
        SDL_Rect dr = {0,0, size_.w,size_.h};
        SDL_RenderCopy(res->renderer, tile, 0, &dr);
        //SDL_Point cr = {}; //{dr.w/2, dr.h/2};
        //SDL_RenderCopyEx(renderer, tile, 0, &dr, 90, &cr, SDL_FLIP_NONE);
    }
    SDL_SetRenderTarget(res->renderer, NULL);
    SDL_SetRenderDrawBlendMode(res->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(res->renderer, 0,0,0, 255);
    //SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_Rect dr = {size_.x-size_.s/2,size_.y-size_.s/2, size_.s,size_.s};
    SDL_RenderCopyEx(res->renderer, texture, 0, &dr,  0, 0, SDL_FLIP_NONE);
    SDL_RenderCopyEx(res->renderer, texture, 0, &dr, 90, 0, SDL_FLIP_NONE);
}

int Main::loop(int argc, char* const argv[])
{
    UInput input(this); //ensure(argc>1, "argc"); SDL_Texture* tils = create_texture(argv[1]);
    SDL_Event ev;

    while (1) {
        view_.Renderer::draw_background();
        view_.draw_test(tiles[0]);
        if (stage_ > 0)
            view_.draw(m_);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    return(0);
                case SDL_KEYUP:
                    switch (int sym = ev.key.keysym.sym) {
                        case SDLK_ESCAPE: // If escape is pressed, return (and thus, quit)
                            return 0;
                        default: input.pressed(sym);//;drawrect(gSurface, {64,48,64,48}, 0x88ff|((sym*3)<<16)&0xffffff);
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                        SDL_Log("SDL_WINDOWEVENT_RESIZED");
                        SDL_SetWindowDisplayMode(window, &displayMode);
                        view_ = View(*this); //view_.window_resized();
                    }
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

