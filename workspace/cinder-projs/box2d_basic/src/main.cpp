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

struct Ev_Quit {};
struct Ev_Pause {};
struct Ev_Resume {};
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

template <typename FSM>
class App_ : public AppNative
{
	Tetris M;
    std::vector<uint8_t> rows_;
    enum class stat {
        normal=0, over=1, pause
    };
	stat stat_;
	unsigned char box_size_;
	
	audio::VoiceRef mVoice;
    //audio::GainNodeRef              mGain;
    //audio::BufferPlayerNodeRef      mBufferPlayerNode;
    FSM fsm_;

public:
    App_() : fsm_(io_service(), M, *this) {}

	void prepareSettings( Settings *settings );
	void setup();

	void mouseDown( MouseEvent event );	
	void keyDown( KeyEvent event );
	void update();

	void draw();
	Vec2i drawMultiArray(Vec2i p, Array2d const& m, bool bg=0);
	Area drawStatus();
    Area drawScore();
    Area drawPreview(Array2d const& pv);
	void play_sound( const char* asset );

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
        if (!M.Move(0)) {
            rows_.push_back(M.last_nclr_);
            if (!M.next_round()) {
                game_over();
            } else if (keydown) {
                play_sound( "speedown.wav" );
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

	void addBox( const Vec2f &pos );
	b2World				*mWorld;
	vector<b2Body*>		mBoxes;

};

void App_::prepareSettings( Settings *settings )
{
    settings->setWindowSize( BOX_SIZE*16, BOX_SIZE*22 );

	//box_size_ = ;
	//extern void msm_test(); 	msm_test();
}

void App_::setup()
{
	b2Vec2 gravity( 0.0f, 5.0f );
	mWorld = new b2World( gravity );

	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set( 0.0f, getWindowHeight() );
	b2Body* groundBody = mWorld->CreateBody(&groundBodyDef);

	// Define the ground box shape.
	b2PolygonShape groundBox;

	// The extents are the half-widths of the box.
	groundBox.SetAsBox( getWindowWidth(), 10.0f );

	// Add the ground fixture to the ground body.
	groundBody->CreateFixture(&groundBox, 0.0f);

	//auto ctx = audio::Context::master();
	//auto sourceFile = /*, ctx->getSampleRate()*/);//(app::loadResource("1.wav"));
//audio::BufferRef buffer = sourceFile->loadBuffer();
//    mBufferPlayerNode = ctx->makeNode( new audio::BufferPlayerNode( buffer ) );
//    // add a Gain to reduce the volume
//    mGain = ctx->makeNode( new audio::GainNode( 0.5f ) );
//    // connect and enable the Context
//    mBufferPlayerNode >> mGain >> ctx->getOutput();
//    ctx->enable();

	//float volume = 1.0f - (float)event.getPos().y / (float)getWindowHeight();
	//float pan = (float)event.getPos().x / (float)getWindowWidth();
	//mVoice->setVolume( volume );
	//mVoice->setPan( pan );

	new_game();
}
void App_::play_sound( const char* asset )
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

void App_::new_game()
{
    rows_.clear();;
	stat_ = stat::normal;
	M.reset(20, 10);  //std::cerr << M << "\n";
	M.next_round();
	// play_sound( "newgame.wav" );
}

void App_::addBox( const Vec2f &pos )
{
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set( pos.x, pos.y );

	b2Body *body = mWorld->CreateBody( &bodyDef );

	b2PolygonShape dynamicBox;
	dynamicBox.SetAsBox( BOX_SIZE, BOX_SIZE );

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.restitution = 0.5f; // bounce

	body->CreateFixture( &fixtureDef );
	mBoxes.push_back( body );
}

void App_::mouseDown( MouseEvent event )
{
    pause();
	//addBox( event.getPos() );
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
			new_game();
        }
		return;
	}
    if (event.getCode() == KeyEvent::KEY_ESCAPE || event.getChar() == 'q') {
        // ask confirm
		quit();
		return;
	}
	if (stat_ != stat::normal) {
        if (stat_ == stat::over) {
            if (microsec_clock::local_time() - M.td_ > seconds(3)) {
                new_game();
            }
		} else if (stat_ == stat::pause) {
			if (event.getCode() == KeyEvent::KEY_SPACE) {
				pause();
			}
		}
		return;
    }
	if (event.getCode() == KeyEvent::KEY_SPACE) {
		pause();
		return;
	}
    if (event.getCode() == KeyEvent::KEY_UP) {
		M.rotate();
		play_sound( "rotate.wav" );
    } else if (event.getCode() == KeyEvent::KEY_LEFT) {
		M.Move(-1);
    } else if (event.getCode() == KeyEvent::KEY_RIGHT) {
		M.Move(1);
    } else if (event.getCode() == KeyEvent::KEY_DOWN) {
        move_down(1);
	}
}

void App_::update()
{
	//for( int i = 0; i < 10; ++i ) mWorld->Step( 1 / 30.0f, 10, 10 );

	if (stat_ != stat::normal)
		return;

	if (microsec_clock::local_time() - M.td_ > milliseconds(900 - get_level()*10 - get_score()/8)) {
        move_down(0);
	}
}

Vec2i App_::drawMultiArray(Vec2i p, Array2d const& m, bool bg)
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

gl::TextureRef make_tex(std::string const& line)
{
    TextLayout layout;
    layout.setFont( Font( "Arial", 32 ) );
    layout.setColor( Color( 1, 1, 0 ) );

    layout.addLine( line );
    //if (stat_ == stat::over) {
    //    //layout.addLine( std::string("synths: ") );
    //} else if (stat_ == stat::pause) {
    //    layout.addLine( std::string("Pause") );
    //}
    return gl::Texture::create( layout.render( true ) );
}

void App_::drawPreview(Array2d const& pv)
{
	auto s = get_shape(pv);
    Array2d a(boost::extents[std::max(4,s[0])][std::max(4,s[1])]);
    or_assign(a, Point(0,0), pv);

    Vec2i endp;
	endp = drawMultiArray(Vec2i::zero(), a, 1);
    endp = drawScore(Vec2i(0, endp.y+10));
}

Area App_::drawScore(Vec2i p)
{
	char sbuf[64];
	sprintf(sbuf, "score: %d", get_score()); // std::itoa(100);
    gl::color( Color::white() );
    gl::TextureRef tex = make_tex(sbuf);
	tex->getAreaTexCoords();
    gl::draw( tex, Vec2f::zero(), );
	return Area(Vec2i(0,0), tex->getSize());
}

Vec2i App_::drawStatus(Vec2i p, std::string const& s)
{
	Vec2i endp;

    if (!s.empty()) //(stat_ != stat::normal)
    {
		gl::TextureRef tex;
        if (stat_ == stat::over) {
            tex = make_tex("Game over");
        } else if (stat_ == stat::pause) {
            tex = make_tex("Pause");
        }
        gl::draw( tex, offset );
    }
    return endp;
}

void App_::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::color( Color( 1, 0.5f, 0.25f ) );

	glPushMatrix();
        gl::translate(5, 5);
        drawMultiArray(M.vmat_, 1);
        gl::translate( (BOX_SIZE+1)*M.p_[1], (BOX_SIZE+1)*M.p_[0] );
        drawMultiArray(M.smat_);
	glPopMatrix();

	boost::array<int,2> s = get_shape(M.vmat_);
	glPushMatrix();
        // gl::translate((BOX_SIZE+1)*s[1]+BOX_SIZE, BOX_SIZE);
        drawPreview(Vec2i((BOX_SIZE+1)*s[1]+BOX_SIZE, BOX_SIZE), M.pv_);

		gl::enableAlphaBlending();
        gl::color( Color::white() );
        drawStatus(Vec2i( ((BOX_SIZE+1)*10 - tex->getWidth())/2, 50 ), "");
		gl::disableAlphaBlending();
	glPopMatrix();

	//for( vector<b2Body*>::const_iterator boxIt = mBoxes.begin(); boxIt != mBoxes.end(); ++boxIt ) {
	//	Vec2f pos( (*boxIt)->GetPosition().x, (*boxIt)->GetPosition().y );
	//	float t = toDegrees( (*boxIt)->GetAngle() );

	//	glPushMatrix();
	//	gl::translate( pos );
	//	gl::rotate( t );

	//	Rectf rect( -BOX_SIZE, -BOX_SIZE, BOX_SIZE, BOX_SIZE );
	//	gl::drawSolidRect( rect );

	//	glPopMatrix();	
	//}
}

struct Main_ : public msm::front::state_machine_def<Main_>
{
    struct Quit : public msm::front::state<>
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

    struct Playing_ : public msm::front::state_machine_def<Playing_>
    {
        struct Action
        {
            template <class Ev, class SM, class SourceState, class TargetState>
            void operator()(Ev const& ev, SM& sm, SourceState&, TargetState&)
            {
                if (!tetris_.Move(ev.how)) {
                    if (ev.how == 0) {
                        rows_.push_back(tetris_.last_rows[0]);
                        if (!tetris_.next_round()) {
                            sm.process_event(Ev_Over());
                        } else if (keydown) {
                            play_sound( "speedown.wav" );
                        }
                    }
                }
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

        typedef Normal initial_state;
        struct transition_table : mpl::vector<
            Row< Normal   ,  Ev_Input    ,  none     ,  Action    ,  none >,
            Row< Normal   ,  Ev_Blink    ,  Blink    ,  none      ,  none >,
            Row< Blink    ,  Ev_EndBlink ,  Normal   ,  none      ,  none >
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

    struct Paused : public msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM&) {}
    }; // Paused

    template <int Ns, class Ev_Idle>
    struct delay_evt
    {
        template <class Ev, class SM, class SourceState, class TargetState>
        void operator()(Ev const& ev ,SM&,SourceState& ,TargetState& )
        {
            std::cout << "&delay_evt "<< typeid(Ev).name() <<"\n";
        }
    };

    /// everybody starts in state 1
    typedef Preview initial_state;

    struct transition_table : mpl::vector<
        Row< Preview  ,  Ev_Quit     ,  Quit     ,  none                  ,  none >,
        Row< Preview  ,  boost::any  ,  Prepare  ,  delay_evt<3,Ev_Idle>  ,  none >,
        Row< Prepare  ,  Ev_Play     ,  Playing  ,  none                  ,  none >,
     // Row< Prepare  ,  Ev_Idle     ,  Preview  ,  none                  ,  none >,
        Row< Prepare  ,  Ev_Quit     ,  Quit     ,  none                  ,  none >,
        Row< Playing  ,  Ev_Pause    ,  Paused   ,  none                  ,  none >,
        Row< Playing  ,  Ev_Quit     ,  Paused   ,  none                  ,  none >,
        Row< Playing  ,  Ev_Over     ,  GameOver ,  none                  ,  none >,
        Row< Paused   ,  Ev_Resume   ,  Playing  ,  none                  ,  none >,
        Row< Paused   ,  Ev_Quit     ,  Quit     ,  none                  ,  none >
    > {};

    template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
        std::cout << "Top entry\n";
    }
    template <class Ev, class SM> void on_exit(Ev const&, SM&) {
        boost::system::error_code ec;
        deadline_.cancel(ec);
        quit_(); //(ev, sm);
        std::cout << "Top exit\n";
    }
    template <class SM, class Ev> void no_transition(Ev const&, SM&, int state)
    {
        std::cout << "Tetris no transition from state " << state
            << " on event " << typeid(Ev).name() << std::endl;
    }

    Main_(boost::asio::io_service& io_s, boost::function<void()> quit)
        : io_s_(io_s)
        , deadline_(io_s)
        , quit_(quit)
    {}

    Tetris tetris_;
    boost::asio::io_service& io_s_;
    boost::asio::deadline_timer deadline_;

    boost::asio::deadline_timer& deadline() { return deadline_; }
    boost::function<void()> quit_;
}; // Main_

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

CINDER_APP_NATIVE( App_<msm::back::state_machine<Main_>>, RendererGl )

