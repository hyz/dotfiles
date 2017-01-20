#include <string>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/type_traits/is_same.hpp>

#include <Box2D/Box2D.h>
#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/audio/Voice.h"
//#include "cinder/audio/Context.h"
//#include "cinder/audio/NodeEffects.h"
//#include "cinder/audio/SamplePlayerNode.h"

// #include "cinder/DataSource.h"
#include "multi_array_tetris.hpp"
#define LOG std::clog

using namespace ci;
using namespace ci::app;
using namespace boost::posix_time;

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
//using namespace std;
//using namespace std;
//using namespace msm::front::euml; // for And_ operator

static const int BOX_SIZE = 40;
static const int BOX_SIZEpx = 41;

struct Ev_Input {
    int a;
    Ev_Input(int x) { a=x; }
};
struct Ev_Back {
    int unpause;
    Ev_Back(int x=0) { unpause=x; }
};
struct Ev_Over {};
struct Ev_Timeout {};
struct Ev_Leave {};
struct Ev_Resume {};
struct Ev_NewGame {};
struct Ev_Menu {};

struct Ev_Blink {};
struct Ev_EndBlink {};

template <class M, class Ev> void emit(M& m, Ev ev);

struct Model : Tetris_Basic
{
    //enum class stat { normal=0, over=1, pause };

    std::vector<round_result> rounds_;
    std::string stats;
    Model() {}

    void reset()
    {
        rounds_.clear();;
        // stat_ = stat::normal;

        Tetris_Basic::reset(20, 10);  //std::cerr << model << "\n";
        Tetris_Basic::next_round();
    }

    int get_score() const
    {
        int score = 0;
        for (auto& x : rounds_) {
            switch (x.size()) {
                case 4: score += 40 * 2; break;
                case 3: score += 30 + 15; break;
                case 2: score += 20 + 10; break;
                case 1: score += 10; break;
            }
        }
        return score;
    }
    int get_level() const { return rounds_.size()/10; }

    bool time2falling(ptime const& tp) const
    {
        return (tp - td_ > milliseconds(900 - get_level()*10 - get_score()/8));
    }
};

struct View
{
    void operator()(Model const& M/*, std::string const& stats*/);

    void play_sound( const char* asset );

    gl::TextureRef make_tex(std::string const& line)
    {
        TextLayout layout;
        layout.setFont( Font( "Arial", 32 ) );
        layout.setColor( Color( 1, 1, 0 ) );

        layout.addLine( line );
        return gl::Texture::create( layout.render( true ) );
    }
    inline Vec2i size(Vec2i bp, Vec2i ep) { return Vec2i(abs(ep.x - bp.x), abs(ep.y - bp.y)); }

    Vec2i drawString(int x, int y, std::string const& v);
    Vec2i drawString(Vec2i bp, Vec2i ep, std::string const& v);
    Vec2i drawArray2d(Vec2i p, Array2d const& m, bool bg);

    void prepareSettings(AppNative::Settings *settings) {
        settings->setWindowSize( BOX_SIZE*16, BOX_SIZE*22 );
    }

    audio::VoiceRef mVoice;
};

Vec2i View::drawString(int x, int y, std::string const& v)
{
    gl::TextureRef tex = make_tex(v); //("score: " + std::to_string(v));
    gl::draw( tex, Vec2i(x,y) );
    return Vec2i(x,y) + tex->getSize(); //Area(Vec2i(0,0), tex->getSize());
}

Vec2i View::drawString(Vec2i bp, Vec2i ep, std::string const& v)
{
    gl::TextureRef tex = make_tex(v); //("score: " + std::to_string(v));

    Vec2i s0 = size(bp, ep);
    Vec2i s1 = tex->getSize();

    int w = s0.x;
    if (s0.x > s1.x) {
        w = (s0.x - s1.x) / 2;
    }
    int h = s0.y;
    if (s0.y > s1.y) {
        h = (s0.y - s1.y) / 2;
    }
    bp += Vec2i(w,h);
    gl::draw( tex, bp );
    return bp + Vec2i(w,h);
}

Vec2i View::drawArray2d(Vec2i p, Array2d const& m, bool bg)
{
    Vec2i endp;
    int bsiz = BOX_SIZE;
    auto const s = get_shape(m);
    for (int y=0; y != s[0]; ++y) {
        for (int x=0; x != s[1]; ++x) {
            Vec2i p( (bsiz+1)*x + p.x , (bsiz+1)*y + p.y );
            endp = p + Vec2i(bsiz, bsiz);
            Rectf rect(p, endp);
            if (m[y][x]) {
                gl::color( Color( 0.6f, 0.3f, 0.15f ) );
            } else if (bg) {
                gl::color( Color( 0.1f, 0.1f, 0.1f ) );
            } else
                continue;
            gl::drawSolidRect( rect );
        }
    }
    return endp;
}

void View::operator()(Model const& M)
{
    gl::clear( Color( 0, 0, 0 ) );
    glPushMatrix();
        // gl::translate(5, 5);
        gl::color( Color( 1, 0.5f, 0.25f ) );

        Vec2i bp(BOX_SIZE,BOX_SIZE); // Area aMx, aPx;
        Vec2i ep = drawArray2d(bp, M.vmat_, 1); // gl::translate( (BOX_SIZE+1)*M.p_[1], (BOX_SIZE+1)*M.p_[0] );

        drawArray2d(Vec2i( bp.x + BOX_SIZEpx*M.p_[1], bp.y + BOX_SIZEpx*M.p_[0] ), M.smat_, 0);

        Array2d pv; {
            auto s = get_shape(M.pv_);
            pv.resize(boost::extents[std::max(4,s[0])][std::max(4,s[1])]);
            or_assign(pv, Point(0,0), M.pv_);
        }
        Vec2i bp2( ep.x + BOX_SIZE, bp.y );
        Vec2i ep2 = drawArray2d(bp2, pv, 1);

        gl::enableAlphaBlending();
        gl::color( Color::white() );
        drawString(bp2.x, ep2.y+BOX_SIZE, "score: " + std::to_string(M.get_score()));
        if (!M.stats.empty())
            drawString(bp, ep, M.stats); //("Game over"); ("Pause");
        gl::disableAlphaBlending();
    glPopMatrix();
}

void View::play_sound( const char* asset )
{
    fs::path p = "sound";
    try {
        if (mVoice)
            mVoice->stop();
        mVoice = audio::Voice::create( audio::load(loadAsset(p/asset)) );

        float volume = 1.0f;
        float pan = 0.5f;

        mVoice->setVolume( volume );
        mVoice->setPan( pan );

        if( mVoice->isPlaying() )
            mVoice->stop();

        mVoice->start();
    } catch (...) {
    }
}

struct PrintS
{
    template <class Ev, class SM, class SS, class TS>
    void operator()(Ev const& ev, SM& sm, SS&, TS&) const
    {
        LOG << " on-ev " << typeid(Ev).name()
            << " state " << typeid(SS).name() <<" "<< typeid(TS).name()
            << "\n";
    }
};

void TheEnd();
struct Main_;
msm::back::state_machine<Main_>& Top();

struct Main_ : msm::front::state_machine_def<Main_> , boost::noncopyable
{
    Model model;
    View view;

    boost::asio::deadline_timer deadline_;
    bool in_playing_;

public:
    Main_(boost::asio::io_service& io_s) : deadline_(io_s)
    {
        in_playing_ = 0;
    }

    void draw() { view(model); }
    void update() {}

    struct Playing_ : msm::front::state_machine_def<Playing_> // , boost::noncopyable
    {
        struct Action
        {
            template <class Ev, class SM, class SS, class TS>
            void operator()(Ev const& ev, SM&, SS&, TS&) {
                auto& top = Top();
                if (is_rotate(ev)) {
                    top.model.rotate();
                    top.view.play_sound( "rotate.wav" );
                    return;
                }
                if (!act(ev, top, "speedown.wav")) {
                    emit(top, Ev_Over());
                }
            }
            template <class Top>
            int act(Ev_Input ev, Top& top, char const* snd)
            {
                if (!top.model.Move(ev.a)) {
                    if (ev.a == 0) {
                        top.model.rounds_.push_back( top.model.last_round_result);
                        if (!top.model.next_round())
                            return 0;
                        if (snd)
                            top.view.play_sound(snd);
                    }
                }
                return 1;
            }
            int is_rotate(Ev_Input ev) { return (ev.a > 1); }
        };
        struct AutoFall : Action
        {
            template <class Ev, class SM, class SS, class TS>
            void operator()(Ev const&, SM&, SS&, TS&) {
                act(Ev_Input(0), Top(), 0);
            }
        };
        struct Busy : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM& ) {}
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        };
        struct Blinking : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
                emit(sm, Ev_EndBlink()); // ;
            }
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        };

        typedef Busy initial_state;
        struct transition_table : mpl::vector<
            Row< Busy     ,  Ev_Input    ,  none     ,  Action    ,  none >,
            Row< Busy     ,  Ev_Timeout  ,  none     ,  AutoFall  ,  none >,
            Row< Busy     ,  Ev_Blink    ,  Blinking ,  none      ,  none >,
            Row< Blinking ,  Ev_EndBlink ,  Busy     ,  none      ,  none >
        > {};

        template <class Ev, class SM> void on_entry(Ev const& ev, SM&) {
            auto& top = Top();
			//if (is_newgame(ev)) {
			//	top.model.reset();
			//	// play_sound( "newgame.wav" );
			//}
            top.in_playing_=1;
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {
            auto& top = Top();
            top.in_playing_=0;
        }
        template <class SM, class Ev> void no_transition(Ev const&, SM&, int state) {
            LOG << "S:Playing no transition on-ev " << typeid(Ev).name() << "\n";
        }

        void autofall()
        {
            //if (model.time2falling(microsec_clock::local_time())) {
            //    emit(te, Ev_Timeout());
            //}
        }

    }; // Playing_

    struct Play_ : msm::front::state_machine_def<Play_>
    {
        //typedef msm::back::state_machine<Playing_,msm::back::ShallowHistory<mpl::vector<Ev_Resume>>> Playing;
        typedef msm::back::state_machine<Playing_> Playing; // back-end

        struct Preview : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM&) {}
            template <class Ev,class SM> void on_exit(Ev const&, SM&) {}
        }; // Preview

        struct GameOver : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM&) {
                Top().model.stats = "Game Over";
            }
            template <class Ev, class SM> void on_exit(Ev const&, SM&) {
				Top().model.stats = "";
			}
        };

        typedef Playing initial_state;
        struct transition_table : mpl::vector<
            Row< Preview  , Ev_Timeout  , Preview  , none  , none    >,
            Row< Preview  , Ev_NewGame  , Playing  , none  , none    >,
            Row< Playing  , Ev_Input    , none     , none  , none    >,
            Row< Playing  , Ev_Over     , GameOver , none  , none    >,
            Row< GameOver , Ev_Input    , Preview  , none  , none    >
            Row< GameOver , Ev_NewGame  , Playing  , none  , none    >
        > {};

        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        template <class SM, class Ev> void no_transition(Ev const& ev, SM& sm, int s) {
            LOG << "S:PlayV  no transition on-ev " << typeid(Ev).name() << "\n";
            if (boost::is_same<Ev_NewGame,Ev>::value) {}
        }
    };
    typedef msm::back::state_machine<Play_> PlayV ; // back-end
    //typedef msm::back::state_machine<Play_,msm::back::ShallowHistory<mpl::vector<Ev_NewGame>>> PlayV ;
	//typedef msm::back::state_machine<Play_,msm::back::AlwaysHistory> PlayV ;
	
    struct Menu : msm::front::state<>
    {
        template <class Ev,class SM> void on_entry(Ev const&, SM& top) {
			top.model.stats = "Menu";
            // sm.io_service().post(boost::bind(&emit<SM,Ev_Restart>, boost::ref(sm), Ev_Restart())); //([&sm]() { emit(sm, Ev_Restart()); });
        }
        template <class Ev,class SM> void on_exit(Ev const&, SM& top) {
			top.model.stats = "";
		}
    }; // Menu

    struct Leave : msm::front::state<>
    {
        template <class Ev,class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev,class SM> void on_exit(Ev const&, SM& ) {}
    }; // Leave

    struct Paused : msm::front::interrupt_state<Ev_Back>
    {
        template <class Ev, class SM> void on_entry(Ev const& ev, SM& top) {
            top.model.stats = "Paused";
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& top) {
            top.model.stats = "";
        }
    }; // Paused
    struct Quit : msm::front::terminate_state<>
    {
        template <class Ev, class SM> void on_entry(Ev const& ev, SM& top) {
            top.io_service().post(&TheEnd);
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& top) {
            // top.model.rotate();
        }
    }; // Quit

    struct Play0 : msm::front::state<> {
        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
            emit(sm, Ev_Resume());
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& sm) {
            //emit(sm, Ev_Leave());
        }
    };

    struct isUnpause {
        template <class Ev, class SM, class SS, class TS>
        bool operator()(Ev const& b, SM&, SS&, TS& ) const {return (b.unpause);}
    };
    struct isPlaying {
        template <class Ev, class SM, class SS, class TS>
        bool operator()(Ev const&, SM& sm, SS&, TS& ) const { return sm.in_playing_; }
    };
    struct Newgame {
        template <class Ev, class SM, class SS, class TS>
        void operator()(Ev const&, SM& sm, SS&, TS&) {
            sm.model.reset();
        }
    };

    typedef mpl::vector<PlayV,Play0> initial_state;

    struct transition_table : mpl::vector<
        Row< PlayV   , boost::any , none    , none    , none        >,
        Row< PlayV   , Ev_Menu    , Menu    , none    , none        >,
        Row< Menu    , Ev_Back    , PlayV   , none    , none        >

        Row< Play0   , Ev_NewGame , Play0   , Newgame , none        >,
        Row< Play0   , Ev_Back    , Quit    , none    , hasLayer    >,
        Row< Play0   , Ev_Back    , Paused  , none    , isPlaying   >,
        Row< Paused  , Ev_Back    , Quit    , none    , none        >,
        Row< Paused  , Ev_Back    , Play0   , none    , isUnpause   >,
    > {};

    template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
        LOG << "S:Main entry\n";
    }
    template <class Ev, class SM> void on_exit(Ev const&, SM&) {
        boost::system::error_code ec;
        deadline_.cancel(ec);
        LOG << "S:Main exit\n";
    }
    template <class SM, class Ev> void no_transition(Ev const&, SM&, int state) {
        LOG << "S:Main no transition on-ev " << typeid(Ev).name() << "\n";
    }
	boost::asio::io_service& io_service() { return deadline_.get_io_service(); }
}; // Main_

static class App_* app_ = 0;

class App_ : public AppNative, boost::noncopyable
{
public:
    msm::back::state_machine<Main_> main_;

    App_() : main_(boost::ref(io_service()))
    {
        app_ = this;
        //main_.set_states( back::states_ << Main_::Playing(boost::ref(main_)) );
    }

    void prepareSettings(Settings *settings) { main_.view.prepareSettings(settings); }
    void setup() { main_.start(); } // state_machine:start
    void update() { main_.update(); }
    void draw() { main_.draw(); }

    void mouseDown( MouseEvent event ) { emit(main_, Ev_Back(1)); }
    void keyDown( KeyEvent event );
};

msm::back::state_machine<Main_>& Top()
{
    return app_->main_;
}
void TheEnd()
{
    app_->main_.stop();
    app_->quit();
};

template <typename SM,typename Ev>
struct PostEv
{
    SM* sm;
    Ev ev;
    PEv(SM& sm_, Ev ev_) : sm(&sm_), ev(ev_) {}
    void operator()() const {
        sm->process_event(ev);
    }
};
template <class M, class Ev> void emit(M& m, Ev ev)
{
    app_->io_service().post( PostEv<M,Ev>(m, ev) );
    //static char const* const state_names[] = {"Preview", "Menu", "Playing", "GameOver", "NonPlaying", "YesPlaying", "Paused", "Quit", "Busy", "Blinking", "", "", ""};
    //LOG << "=B " << state_names[m.current_state()[0]] << " Ev:"<< typeid(Ev).name() <<"\n";
    //m.process_event(ev);
    //LOG << "=E " << state_names[m.current_state()[0]] <<"\n";
}

void App_::keyDown( KeyEvent event )
{
#if defined( CINDER_COCOA )
    bool isModDown = event.isMetaDown();
#else // windows
    bool isModDown = event.isControlDown();
#endif

    if (isModDown) {
        if( event.getChar() == 'n' ) {
            emit(main_, Ev_NewGame());
            // } else if (event.getChar() == 'q') { emit(main_, Ev_Back(1));
        }
        return;
    }

    int ev = 0;
    switch (event.getCode()) {
        case KeyEvent::KEY_ESCAPE: emit(main_, Ev_Back()); return;
        case KeyEvent::KEY_SPACE: emit(main_, Ev_Menu()); return;
        case KeyEvent::KEY_UP: ev = 2; break;
        case KeyEvent::KEY_LEFT: ev = -1; break;
        case KeyEvent::KEY_RIGHT: ev = 1; break;
        case KeyEvent::KEY_DOWN: ev = 0; break;
        default: return;
    }
    emit(main_, Ev_Input(ev));
}

//int _testmain()
//{
//    boost::asio::io_service io_s;
//    Main te(boost::ref(io_s));
//
//    emit(te, Ev_Input(1));
//    emit(te, Ev_NewGame());
//    emit(te, Ev_Input(0));
//    emit(te, Ev_Blink());
//    emit(te, Ev_Back());
//  //emit(te, Ev_Resume());
//    emit(te, Ev_Input(1));
//    emit(te, Ev_EndBlink());
//    emit(te, Ev_Input(0));
//    emit(te, Ev_Blink());
//    emit(te, Ev_Input(1));
//    emit(te, Ev_EndBlink());
//    emit(te, Ev_Input(0));
//    emit(te, Ev_Quit());
//    emit(te, Ev_Quit());
//  //emit(te, Ev_Quit());
//
//    return 0;
//}


//    void VoiceBasicApp::mouseDown( MouseEvent event )
//{
//// scale volume and pan from window coordinates to 0:1
//float volume = 1.0f - (float)event.getPos().y / (float)getWindowHeight();
//float pan = (float)event.getPos().x / (float)getWindowWidth();
//mVoice->setVolume( volume );
//mVoice->setPan( pan );
//// By stopping the Voice first if it is already playing, start will play from the beginning
//if( mVoice->isPlaying() )
//mVoice->stop();
//mVoice->start();
//}
//void VoiceBasicApp::keyDown( KeyEvent event )
//{
//// space toggles the voice between playing and pausing
//if( event.getCode() == KeyEvent::KEY_SPACE ) {
//if( mVoice->isPlaying() )
//mVoice->pause();
//else
//mVoice->start();
//}
//}

CINDER_APP_NATIVE( App_, RendererGl )

