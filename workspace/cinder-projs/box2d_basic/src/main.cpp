#include <string>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

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
#include "multi_array_tetris.h"

using namespace ci;
using namespace ci::app;
using namespace boost::posix_time;

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
//using namespace std;
//using namespace std;
//using namespace msm::front::euml; // for And_ operator

const float BOX_SIZE = 40;
const float BOX_SIZEp1 = 41;

struct Ev_Quit {};
struct Ev_Pause {};
struct Ev_Resume {};
struct Ev_Restart {};
struct Ev_Over {};
struct Ev_Play {};
struct Ev_Idle {};
struct Ev_Input {
    int how;
    Ev_Input(int x) { how=x; }
};
struct Ev_Blink {};
struct Ev_EndBlink {};
template <class M, class Ev> void do_event(M& m, Ev const& ev);

struct View
{
    void operator()(Tetris const& M);

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
    Vec2i drawMultiArray(Vec2i p, Array2d const& m, bool bg);

    void prepareSettings(Settings *settings) {
        settings->setWindowSize( BOX_SIZE*16, BOX_SIZE*22 );
    }

	audio::VoiceRef mVoice;
};

Vec2i View::drawString(int x, int y, std::string const& v)
{
    gl::TextureRef tex = make_tex(v); //("score: " + std::to_string(v));
    gl::draw( tex, Vec2i(x,y) );
	return p + tex->getSize(); //Area(Vec2i(0,0), tex->getSize());
}

Vec2i View::drawString(Vec2i bp, Vec2i ep, std::string const& v)
{
    gl::TextureRef tex = make_tex(v); //("score: " + std::to_string(v));

    Vec2i s0 = size(bp, ep);
    Vec2i s1 = tex.getSize();

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

Vec2i View::drawMultiArray(Vec2i p, Array2d const& m, bool bg)
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

void View::operator()(Tetris const& M)
{
	gl::clear( Color( 0, 0, 0 ) );
	glPushMatrix();
        // gl::translate(5, 5);
        gl::color( Color( 1, 0.5f, 0.25f ) );

        Vec2i bp(BOX_SIZE,BOX_SIZE); // Area aMx, aPx;
        Vec2i ep = drawMultiArray(bp, M.vmat_, 1); // gl::translate( (BOX_SIZE+1)*M.p_[1], (BOX_SIZE+1)*M.p_[0] );

        drawMultiArray(Vec2i( bp.x + BOX_SIZEp1*M.p_[1], bp.y + BOX_SIZEp1*M.p_[0] ), M.smat_);

        Array2d pv; {
            auto s = get_shape(M.pv_);
            pv.resize(boost::extents[std::max(4,s[0])][std::max(4,s[1])]);
            or_assign(pv, Point(0,0), M.pv_);
        }
        Vec2i bp2( ep.x + BOX_SIZE, bp.y );
        Vec2i ep2 = drawMultiArray(bp2, pv, 1);

		gl::enableAlphaBlending();
        gl::color( Color::white() );
        drawString(bp2.x, ep2.y+BOX_SIZE, "score: " + std::to_string(get_score()));
        drawString(bp, ep, state); //("Game over"); ("Pause");
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

struct Main_ : msm::front::state_machine_def<Main_>
{
    Tetris model;
    View view;

    std::vector<uint8_t> rows_;
    enum class stat {
        normal=0, over=1, pause
    };
	stat stat_;
	// unsigned char box_size_;
	
    boost::asio::deadline_timer deadline_;

public:
    Main_(boost::asio::io_service& io_s) : io_s_(io_s) , deadline_(io_s_)
    {}

    void draw() { view(model); }
    void update();

	void new_game();
    void pause()
    {
        if (stat_ == stat::normal)
            stat_ = stat::pause;
        else if (stat_ == stat::pause)
            stat_ = stat::normal;
    }
    void move_down(bool keydown)
    {
        if (!model.Move(0)) {
            rows_.push_back(model.last_nclr_);
            if (!model.next_round()) {
                game_over();
            } else if (keydown) {
                view.play_sound( "speedown.wav" );
            }
        }
    }
	void game_over() {
		stat_= stat::over;
		//play_sound( "gameover.wav" );
	}

    int get_score() const
    {
        int score = 0;
        for (int x : rows_) {
            switch (x) {
                case 4: score += 40 * 2; break;
                case 3: score += 30 + 15; break;
                case 2: score += 20 + 10; break;
                case 1: score += 10; break;
            }
        }
        return score;
    }
    int get_level() const
    {
        return rows_.size()/10; //(get_score()+90) / 100;
    }

//  }; struct Main_ : public msm::front::state_machine_def<Main_> {

    struct Quit : msm::front::state<> 
        // : msm::front::terminate_state<>
        // : msm::front::interrupt_state<> // <boost::any>
    {
        template <class Ev, class SM> void on_entry(Ev const& ev, SM& sm)
        {
            sm.stop();
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM&) {}
    }; // Quit
    struct Preview : public msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const&, SM& ) {
            ;
        }
        template <class Ev,class SM> void on_exit(Ev const&, SM& ) {}
    }; // Preview

    struct Prepare : public msm::front::state<>
    {
        template <class Ev,class SM> void on_entry(Ev const&, SM& ) {
            sm.process_event(Ev_Play()); // ;
        }
        template <class Ev,class SM> void on_exit(Ev const&, SM& ) {}
    }; // Prepare

    struct Running : public msm::front::state<> {};

    struct Paused : msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM&) {}
    }; // Paused

    struct PrintS
    {
        template <class Ev, class SM, class SourceState, class TargetState>
        void operator()(Ev const& ev, SM& sm, SourceState& ss, TargetState&)
        {
            std::cout << "from state " << typeid(ss).name() << std::endl;
        }
    };

    struct Playing_ : public msm::front::state_machine_def<Playing_>
    {
        struct Action
        {
            template <class Ev, class SM, class SourceState, class TargetState>
            void operator()(Ev const& ev, SM& sm, SourceState&, TargetState&)
            {
                if (!sm.model.Move(ev.how)) {
                    if (ev.how == 0) {
                        rows_.push_back(sm.model.last_rows[0]);
                        if (!sm.model.next_round()) {
                            sm.process_event(Ev_Over());
                        } else if (keydown) {
                            view.play_sound( "speedown.wav" );
                        }
                    }
                }
                //sm.view.play_sound( "rotate.wav" );
            }
        };
        struct Normal : public msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM& ) {}
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        };
        struct Blink : public msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
                sm.process_event(Ev_EndBlink()); // ;
            }
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        };

        typedef Running initial_state;
        struct transition_table : mpl::vector<
            Row< Running  ,  Ev_Input    ,  none     ,  Action    ,  none >,
            Row< Running  ,  Ev_Blink    ,  Blink    ,  none      ,  none >,
            Row< Running  ,  Ev_Pause    ,  Paused   ,  none      ,  none >,
            Row< Running  ,  Ev_Quit     ,  Paused   ,  none      ,  none >,
            Row< Blink    ,  Ev_EndBlink ,  Running  ,  none      ,  none >,
            Row< Paused   ,  Ev_Quit     ,  Quit     ,  PrintS    ,  none >,
            Row< Paused   ,  boost::any  ,  Running  ,  none      ,  none >
        > {};
        //struct internal_transition_table : mpl::vector<
        //    Internal< Ev_Input, Move, none >
        //> {};

        template <class Ev, class SM> void on_entry(Ev const&, SM& ) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        template <class SM, class Ev> void no_transition(Ev const&, SM&, int state)
        {
            std::cout << "Playing no transition from state " << state
                << " on event " << typeid(Ev).name() << std::endl;
        }
    }; // Playing_

    // back-end
    typedef msm::back::state_machine<Playing_,msm::back::ShallowHistory<mpl::vector<Ev_Resume>>> Playing;

    template <int Ns, class Ev_Idle>
    struct delay_evt
    {
        template <class Ev, class SM, class SourceState, class TargetState>
        void operator()(Ev const& ev ,SM&,SourceState& ,TargetState& )
        {
            std::cout << "&delay_evt "<< typeid(Ev).name() <<"\n";
        }
    };

    struct ExclQ
    {
        bool select(Ev_Quit) const { return false; }
        bool select(...) const { return true; }
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM&, SourceState&, TargetState& )
            { return this->select(evt); }
    };

    /// everybody starts in state 1
    // typedef Preview initial_state;
    typedef mpl::vector<Preview,Running> initial_state;

    struct transition_table : mpl::vector<
      //Row< Preview  ,  Ev_Quit     ,  Quit     ,  none                  ,  none     >,
        Row< Preview  ,  boost::any  ,  Prepare  ,  delay_evt<3,Ev_Idle>  ,  ExclQ    >,
        Row< Prepare  ,  Ev_Play     ,  Playing  ,  none                  ,  none     >,
      //Row< Prepare  ,  Ev_Idle     ,  Preview  ,  none                  ,  none     >,
      //Row< Prepare  ,  Ev_Quit     ,  Quit     ,  none                  ,  none     >,
      //Row< Playing  ,  Ev_Pause    ,  Paused   ,  none                  ,  none     >,
      //Row< Playing  ,  Ev_Quit     ,  Paused   ,  none                  ,  none     >,
        Row< Playing  ,  Ev_Over     ,  GameOver ,  none                  ,  none     >,
        Row< Playing  ,  Ev_Restart  ,  Playing  ,  none                  ,  none     >,
        Row< Running  ,  Ev_Quit     ,  Quit     ,  PrintS                ,  none     >
    > {};

    template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
        std::cout << "Top entry\n";
    }
    template <class Ev, class SM> void on_exit(Ev const&, SM&) {
        boost::system::error_code ec;
        deadline_.cancel(ec);
        std::cout << "Top exit\n";
    }
    template <class SM, class Ev> void no_transition(Ev const&, SM&, int state)
    {
        std::cout << "Tetris no transition from state " << state
            << " on event " << typeid(Ev).name() << std::endl;
    }
}; // Main_

void Main_::new_game()
{
    rows_.clear();;
	stat_ = stat::normal;
	model.reset(20, 10);  //std::cerr << model << "\n";
	model.next_round();
	// play_sound( "newgame.wav" );
}

void Main_::update()
{
	if (stat_ != stat::normal)
		return;

	if (microsec_clock::local_time() - model.td_ > milliseconds(900 - get_level()*10 - get_score()/8)) {
        move_down(0);
	}
}

class App_ : public AppNative
{
    msm::back::state_machine<Main_> main_;
public:
    App_() : main_(io_service())
    {}

	void prepareSettings( Settings *settings ) {
        main_.view.prepareSettings(BOX_SIZE*16, BOX_SIZE*22 );
    }
	void setup() { main_.new_game(); }
	void update() { main_.update(); }
	void draw() { main_.draw(); }

	void mouseDown( MouseEvent event ) { main_.pause(); }

	void keyDown( KeyEvent event );
};

void App_::keyDown( KeyEvent event )
{
#if defined( CINDER_COCOA )
    bool isModDown = event.isMetaDown();
#else // windows
    bool isModDown = event.isControlDown();
#endif
    if (isModDown) {
		if( event.getChar() == 'n' ) {
            main_.process_event(Ev_Restart()); // main_.new_game();
        }
		return;
	}
    if (event.getCode() == KeyEvent::KEY_ESCAPE || event.getChar() == 'q') {
        main_.process_event(Ev_Quit()); // main_.quit(); // confirm?
		return;
	}

	if (stat_ != stat::normal) {
        if (stat_ == stat::over) {
            if (microsec_clock::local_time() - model.td_ > seconds(3)) {
                main_.process_event(Ev_Restart());
            }
		} else if (stat_ == stat::pause) {
			if (event.getCode() == KeyEvent::KEY_SPACE) {
                main_.process_event(Ev_Pause()); // main_.pause();
			}
		}
		return;
    }
	if (event.getCode() == KeyEvent::KEY_SPACE) {
        main_.process_event(Ev_Pause()); // main_.pause();
		return;
	}

    int ev = 0;
    switch (event.getCode()) {
        case KeyEvent::KEY_UP: ev = 2; break;
        case KeyEvent::KEY_LEFT: ev = -1; break;
        case KeyEvent::KEY_RIGHT: ev = 1; break;
        case KeyEvent::KEY_DOWN: ev = 0; break;
        default: return;
    }
    main_.process_event(Ev_Input(ev));
}

template <class Ev>
void do_event(Main& t, Ev const& ev)
{
    static char const* const state_names[] = { "Preview", "Prepare", "Playing", "Paused", "Quit" };
    std::cout << "=B " << state_names[t.current_state()[0]] 
        << " <>"<< typeid(Ev).name() <<"\n";
    t.process_event(ev);
    std::cout << "=E " << state_names[t.current_state()[0]] << "\n";
}

int _testmain()
{
    boost::asio::io_service io_s;
    Main te(boost::ref(io_s));

    do_event(te, Ev_Input(1));
    do_event(te, Ev_Play());
    do_event(te, Ev_Input(0));
    do_event(te, Ev_Blink());
    do_event(te, Ev_Pause());
    do_event(te, Ev_Resume());
    do_event(te, Ev_Input(1));
    do_event(te, Ev_EndBlink());
    do_event(te, Ev_Input(0));
    do_event(te, Ev_Blink());
    do_event(te, Ev_Input(1));
    do_event(te, Ev_EndBlink());
    do_event(te, Ev_Input(0));
    do_event(te, Ev_Quit());
    do_event(te, Ev_Quit());
  //do_event(te, Ev_Quit());

    return 0;
}


//	void VoiceBasicApp::mouseDown( MouseEvent event )
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

