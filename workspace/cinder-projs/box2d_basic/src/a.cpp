#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
using namespace std;
//using namespace msm::front::euml; // for And_ operator

struct Ev_Quit {};
struct Ev_Pause {};
struct Ev_Resume {};
struct Ev_Play {};
struct Ev_Idle {};
struct Ev_Input {
    int how;
    Ev_Input(int x) { how=x; }
};

        struct Ev_Blink {};
        struct Ev_EndBlink {};

struct Tetris_ : public msm::front::state_machine_def<Tetris_>
{
    struct Quit : public msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const& ev, SM& sm)
        {
            std::cout << "stopping ...\n";
            sm.stop();
            std::cout << "stopped\n";
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM&) { std::cout << "Quit exit\n"; }
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
        template <class Ev,class SM> void on_entry(Ev const&, SM& ) {}
        template <class Ev,class SM> void on_exit(Ev const&, SM& ) {}
    }; // Prepare

    struct Running : public msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const&, SM& ) {cout<<"Running:entry\n";}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {cout<<"Running:exit\n";}
    };

    struct Paused : public msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM&) {}
    }; // Paused

    struct Playing_ : public msm::front::state_machine_def<Playing_>
    {
        struct Blink : public msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM& ) {cout<<"Blink:entry\n";}
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {cout<<"Blink:exit\n";}
        };
        struct Act
        {
            template <class Ev, class SM, class SourceState, class TargetState>
            void operator()(Ev const& ev, SM& sm, SourceState&, TargetState&)
            {
                std::cout << "&P:Act "<< typeid(Ev).name() <<" "<< ev.how <<"\n";
            }
        };
        struct ActP
        {
            template <class Ev, class SM, class SourceState, class TargetState>
            void operator()(Ev const& ev, SM& sm, SourceState&, TargetState&)
            {
                std::cout << "&P:ActP on-ev "<< typeid(Ev).name() <<" emit Ev_Pause\n";
                sm.process_event(Ev_Pause());
            }
        };

        typedef Running initial_state;
        struct transition_table : mpl::vector<
            Row< Running        ,  Ev_Input    ,  none     ,  Act       ,  none >,
            Row< Running        ,  Ev_Blink    ,  Blink    ,  none      ,  none >,
            Row< Running        ,  Ev_Quit     ,  Running  ,  ActP      ,  none >,
            Row< Blink          ,  Ev_EndBlink ,  Running  ,  none      ,  none >
        > {};
        //struct internal_transition_table : mpl::vector<
        //    Internal< Ev_Input, Act, none >
        //> {};

        template <class Ev, class SM> void on_entry(Ev const&, SM& ) {cout<<"Playing:entry\n";}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {cout<<"Playing:exit\n";}
        template <class SM, class Ev> void no_transition(Ev const&, SM&, int state)
        {
            std::cout << "Playing no transition from state " << state
                << " on-ev " << typeid(Ev).name() << std::endl;
        }
    }; // Playing_
    // back-end
    //typedef msm::back::state_machine<Playing_,msm::back::ShallowHistory<mpl::vector<end_pause> > > Playing;
    //typedef msm::back::state_machine<Playing_> Playing;
    typedef msm::back::state_machine<Playing_,msm::back::ShallowHistory<mpl::vector<Ev_Resume>>> Playing;

    struct Verify
    {
        template <class Ev, class SM, class SourceState, class TargetState>
        bool operator()(Ev const& ev, SM&, SourceState&, TargetState& )
        {
            std::cout << "?Verify\n";
            return false;
        }
    };

    template <int Ns, class Ev_Idle>
    struct InS_evt
    {
        template <class Ev, class SM, class SourceState, class TargetState>
        void operator()(Ev const& ev ,SM&,SourceState& ,TargetState& )
        {
            std::cout << "&InS_evt "<< typeid(Ev).name() <<"\n";
        }
    };
    struct ExclQ
    {
        bool select(Ev_Quit) const { return false; }
        template <typename T> bool select(T) const { return true; }
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(EVT const& evt, FSM&, SourceState&, TargetState& ) { return this->select(evt); }
    };
    struct PrintS
    {
        template <class Ev, class SM, class SourceState, class TargetState>
        void operator()(Ev const& ev, SM& sm, SourceState& ss, TargetState&)
        {
            std::cout << "&PrintS on-ev " << typeid(ev).name()
                << " fr-state " << typeid(ss).name() << std::endl;
        }
    };

    //typedef Preview initial_state;
    typedef mpl::vector<Preview,Running> initial_state;

    struct transition_table : mpl::vector<
        Row< Preview  ,  boost::any  ,  Prepare  ,  InS_evt<3,Ev_Idle>  ,  ExclQ   >,
        Row< Prepare  ,  Ev_Play     ,  Playing  ,  none                ,  none    >,
        Row< Playing  ,  Ev_Pause    ,  Paused   ,  PrintS              ,  none    >,
        Row< Paused   ,  Ev_Resume   ,  Playing  ,  none                ,  none    >,
        Row< x  ,  Ev_Quit     ,  Quit     ,  PrintS              ,  none    >
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
        std::cout << "Tetris_ no transition from state " << state
            << " on event " << typeid(Ev).name() << std::endl;
    }

    Tetris_(boost::asio::io_service& io_s, boost::function<void()> quit)
        : io_s_(io_s)
        , deadline_(io_s)
        , quit_(quit)
    {}

    boost::asio::io_service& io_s_;
    boost::asio::deadline_timer deadline_;

    boost::asio::deadline_timer& deadline() { return deadline_; }
    boost::function<void()> quit_;
}; // Tetris_

typedef msm::back::state_machine<Tetris_> Tetris;

template <class Ev>
void do_event(Tetris& t, Ev const& ev)
{
    static char const* const state_names[] = { "Preview", "Prepare", "Playing", "Paused", "Quit" };
    std::cout << "=B " << state_names[t.current_state()[0]] 
        << " <>"<< typeid(Ev).name() <<"\n";
    t.process_event(ev);
    std::cout << "=E " << state_names[t.current_state()[0]] << "\n";
}

void ending() {cout<<"ending\n";}

int main()
{
    boost::asio::io_service io_s;
    Tetris te(boost::ref(io_s), ending);

    do_event(te, Ev_Input(1)); // Preview -> Prepare
    do_event(te, Ev_Play()); // Prepare -> Playing
    //do_event(te, Ev_Input(0)); // Playing
    //do_event(te, Ev_Blink()); // Playing
    //do_event(te, Ev_Pause()); // Playing -> Paused
    //do_event(te, Ev_Resume()); // Paused -> Playing
    //do_event(te, Ev_Input(1)); // Playing
    //do_event(te, Ev_EndBlink()); // Playing
    do_event(te, Ev_Input(0)); // Playing
    do_event(te, Ev_Blink()); // Playing
    do_event(te, Ev_Input(1)); // Playing
    do_event(te, Ev_EndBlink()); // Playing
    do_event(te, Ev_Input(0)); // Playing
    do_event(te, Ev_Quit()); // Playing - Playing, emit Ev_Pause, -> Paused
    do_event(te, Ev_Quit());  // Paused -> Quit
  //do_event(te, Ev_Quit());

    return 0;
}

