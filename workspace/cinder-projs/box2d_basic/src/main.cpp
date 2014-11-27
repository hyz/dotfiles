#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#include "multi_array_tetris.h"
#include "box2d_basicApp.cpp"

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
//using namespace std;
//using namespace msm::front::euml; // for And_ operator

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

    Tetris_ tetris_;
    boost::asio::io_service& io_s_;
    boost::asio::deadline_timer deadline_;

    boost::asio::deadline_timer& deadline() { return deadline_; }
    boost::function<void()> quit_;
}; // Main_

typedef msm::back::state_machine<Main_> Main;

template <class Ev>
void do_event(Main& t, Ev const& ev)
{
    static char const* const state_names[] = { "Preview", "Prepare", "Playing", "Paused", "Quit" };
    std::cout << "=B " << state_names[t.current_state()[0]] 
        << " <>"<< typeid(Ev).name() <<"\n";
    t.process_event(ev);
    std::cout << "=E " << state_names[t.current_state()[0]] << "\n";
}

void ending() {std::cout<<"ending\n";}

int main()
{
    boost::asio::io_service io_s;
    Main te(boost::ref(io_s), ending);

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

CINDER_APP_NATIVE( Main, RendererGl )

