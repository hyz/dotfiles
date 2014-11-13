#include <iostream>
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
struct Ev_Input {};

struct Tetris_ : public msm::front::state_machine_def< Tetris_ >
{
    struct Preview : public msm::front::state<>
    {
        //struct internal_transition_table : mpl::vector<
        //    Internal< boost::any, Act_Pv, none >
        //> {};

        template <class Ev, class SM>
        void on_entry(Ev const&, SM& ) {}
        template <class Ev,class SM>
        void on_exit(Ev const&, SM& ) {}
    }; // Preview


    struct Prepare : public msm::front::state<>
    {
        template <class Ev,class SM>
        void on_entry(Ev const&,SM& ) {}
        template <class Ev,class SM>
        void on_exit(Ev const&,SM& ) {}
    }; // Prepare

    struct Playing : public msm::front::state<>
    {
        struct Act
        {
            template <class Ev, class SM, class SourceState, class TargetState>
            void operator()(Ev const& ev, SM& sm, SourceState&, TargetState&)
            {
                std::cout << "&Playing:Act "<< typeid(Ev).name() <<"\n";
            }
        };
        struct internal_transition_table : mpl::vector<
            Internal< Ev_Input, Act, none >
        > {};

        template <class Ev, class SM>
        void on_entry(Ev const&, SM& ) {}
        template <class Ev, class SM>
        void on_exit(Ev const&, SM& ) {}
    }; // Playing

    struct Quit : public msm::front::state<>
    {
        template <class Ev, class SM>
        void on_entry(Ev const&, SM& sm) {
            sm.stop();
            std::cout << "bye.\n";
        }
        template <class Ev, class SM>
        void on_exit(Ev const&, SM&) {}
    }; // Prepare

    struct Paused : public msm::front::state<>
    {
        template <class Ev, class SM>
        void on_entry(Ev const&, SM& sm) {}
        template <class Ev, class SM>
        void on_exit(Ev const&, SM&) {}
    }; // Paused

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
    struct InS_pev
    {
        template <class Ev, class SM, class SourceState, class TargetState>
        void operator()(Ev const& ev ,SM&,SourceState& ,TargetState& )
        {
            std::cout << "&InS_pev "<< typeid(Ev).name() <<"\n";
        }
    };

    /// everybody starts in state 1
    typedef Preview initial_state;

    struct transition_table : mpl::vector<
        Row< Preview  ,  Ev_Quit     ,  Quit     ,  none                ,  none >,
        Row< Preview  ,  boost::any  ,  Prepare  ,  InS_pev<3,Ev_Idle>  ,  none >,
        Row< Prepare  ,  Ev_Play     ,  Playing  ,  none                ,  none >,
     // Row< Prepare  ,  Ev_Idle     ,  Preview  ,  none                ,  none >,
        Row< Prepare  ,  Ev_Quit     ,  Quit     ,  none                ,  none >,
        Row< Playing  ,  Ev_Pause    ,  Paused   ,  none                ,  none >,
        Row< Playing  ,  Ev_Quit     ,  Paused   ,  none                ,  none >,
        Row< Paused   ,  Ev_Resume   ,  Playing  ,  none                ,  none >,
        Row< Paused   ,  Ev_Quit     ,  Quit     ,  none                ,  none >
    > {};

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

int main()
{
    Tetris te;
    do_event(te, Ev_Input());
    do_event(te, Ev_Play());
    do_event(te, Ev_Input());
    do_event(te, Ev_Pause());
    do_event(te, Ev_Resume());
    do_event(te, Ev_Input());
    do_event(te, Ev_Input());
    do_event(te, Ev_Quit());
    do_event(te, Ev_Quit());
  //do_event(te, Ev_Quit());

    return 0;
}

