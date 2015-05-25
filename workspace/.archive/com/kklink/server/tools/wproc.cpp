#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>
#include <ftw.h>
#include <signal.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/range.hpp>
#include <boost/algorithm/string.hpp>
// #include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

using namespace boost;
using namespace std;

#define NOTIFY_PROG "sms.php"

typedef char* cstring;

template <typename T>
std::ostream& operator<<(std::ostream& out, const vector<T>& v)
{
    for (auto & t : v)
        cout << t << " "; // for_each(v.begin(), v.end(), [](const T& t){ cout << t << " "; } );
    return out; // << p.id() << ":" << p.path();
}

template <typename Fn, typename T>
void stringv(const Fn& fn, T& arg)
{
    fn( lexical_cast<string>(arg) );
}

template <typename Fn, typename T, typename... Args>
void stringv(const Fn& fn, T& arg, Args... args)
{
    fn( lexical_cast<string>(arg) );
    return stringv(fn, args...);
}

vector<string> path_split(const string& path)
{
    vector<iterator_range<string::const_iterator>> ires;
    split(ires, path, [](char x){return x=='/';}, token_compress_on);

    vector<string> res;
    if (empty(ires[0]))
        res.push_back("/");

    for (auto & r: ires)
        if (!empty(r))
        {
            //if (equals(r, "."))
            //    continue;
            // if (equals(r, ".."))
            // {
            //     res.pop_back();
            //     continue;
            // }
            res.push_back(string(std::begin(r),std::end(r)));
        }
    return res;
}

string path_leaf(const string& path)
{
    vector<iterator_range<string::const_iterator>> res;
    split(res, path, [](char x){return x=='/';}, token_compress_on);

    while (!empty(res) && empty(*res.rbegin()))
        res.pop_back();
    if (empty(res))
        return string();

    auto i = res.rbegin();
    return string(std::begin(*i), std::end(*i));
}

template <typename... Args>
pid_t spawn(const std::string& fsp, Args... args)
{
    vector<string> argv{ fsp, path_leaf(fsp) };
    stringv([&argv](const string& s){ argv.push_back(s); } , args...);

    cout << "spawn: " << argv << endl;

    pid_t pid = fork();
    if (pid < 0)
    {
        return pid;
    }

    if (pid == 0)
    {
        vector<char*> av;
        for (auto & a : argv)
            av.push_back(const_cast<char*>(a.c_str()));
        cerr << av << endl;

        av.push_back(0);
        int x = execv(av[0], &av[1]);

        cerr << x << " execv fail\n";
        exit(123);
    }

    return pid;
}

// template <typename Fn>
// void walk(const filesystem::path& path, const Fn& fn)
// {
//     if (filesystem::is_directory(path))
//     {
//         for_each(filesystem::directory_iterator(path), filesystem::directory_iterator(),
//                 [&fn](boost::filesystem::directory_entry& it) {
//                     walk(it.path(), fn);
//                 });
//     }
//     else if (filesystem::is_regular(path)) // filesystem::is_symlink(path) 
//     {
//         // cout << path << endl;
//         fn(path);
//     }
//     else
//     {
//         cout << "Unknown " << path << endl;
//     }
// }

struct notify
{
    notify(iterator_range<cstring const*> r)
        : phones_(r)
    { stime_ = 0; }

    void add(const string& p);
    void send() ;

    bool empty() const { return paths_.empty(); }

private:
    iterator_range<cstring const*> phones_;
    vector<string> paths_;
    time_t stime_;
};

void notify::add(const string& p)
{
    paths_.erase(remove_if(paths_.begin(), paths_.end(), [&p](string& x){return x==p;})
            , paths_.end());

    size_t sum = 0;
    for (auto & s : paths_)
        sum += s.size();
    if (sum > 140)
        paths_.erase(paths_.begin());

    paths_.push_back(p);
}

void notify::send()
{
    if (!paths_.empty()
            && time(0) - stime_ > 5)
    {
        stime_ = time(0);

        string msg = join(paths_, ";");
        paths_.clear();

        if (fork() == 0)
        {
            close(2);
            open("/dev/null", O_WRONLY);

            vector<char*> av{ const_cast<char*>(NOTIFY_PROG), const_cast<char*>(msg.c_str()) };
            for (auto & p : phones_)
                av.push_back( p );

            av.push_back( 0 );
            execvp(NOTIFY_PROG, &av[0]);

            cerr << NOTIFY_PROG << " fail\n";
            exit(12);
        }
    }
}

// template <typename... Args>
// void notify(iterator_range<cstring const*> r, Args... args)
// {
//     string msg;
//     {
//         vector<string> v;
//         stringv([&v](const string& s){ v.push_back(s); }, args...);
//         msg = join(v, " ");
//     }
// 
//     vector<char*> av{ const_cast<char*>(NOTIFY_PROG), const_cast<char*>(msg.c_str()) };
//     for (auto i = std::begin(r) ; i != std::end(r); ++i)
//     {
//         av.push_back( *i );
//     }
//     cout << "notify:" << av << endl;
// 
//     if (fork() == 0)
//     {
//         close(2);
//         open("/dev/null", O_WRONLY);
// 
//         av.push_back( 0 );
//         execvp(NOTIFY_PROG, &av[0]);
// 
//         cerr << NOTIFY_PROG << " fail\n";
//         exit(12);
//     }
// }

struct proc
{
    std::string path_; // filesystem::path path_;
    pid_t pid_;

    explicit proc(const std::string& p) : path_(p) { pid_ = 0; }
    proc() { pid_ = 0; }

    pid_t start();
    void  stop();

    pid_t id() const { return pid_; }
    const std::string& path() const { return path_; }
};

inline std::ostream& operator<<(std::ostream& out, const proc& p)
{
    return out << p.id() << ":" << p.path();
}

pid_t proc::start()
{
    pid_t pid = spawn(path_, "start"); //(path_, "start")
    if (pid > 0)
    {
        pid_ = pid;
        cout << *this << " started" << endl;
    }
    return pid;
}

void proc::stop()
{
    if (pid_ > 0)
    {
        cout << *this << " stopped" << endl;
        // spawn(path_, "stop");

        kill(pid_, SIGTERM);
        kill(pid_, SIGKILL);
        pid_ = 0;
    }
}

// int runs(const filesystem::path& path, std::vector<proc>& procs)
// {
//     vector<proc> oldps(move(procs));
//     vector<proc> newps;
//     BOOST_ASSERT(empty(procs));
// 
//     walk(path, [&oldps,&newps,&procs](const filesystem::path& path) {
//             auto i = find_if(oldps.begin(), oldps.end()
//                     , [&path](proc& p) { return p.path() == path; });
//             if (i == oldps.end()) {
//                 newps.push_back(proc(path));
//                 // newps.insert(procs.end(), proc(path));
//             } else {
//                 cout << *i << endl;
//                 procs.insert(procs.end(), *i);
//                 oldps.erase(i);
//             }
//         });
// 
//     for_each(oldps.begin(), oldps.end(), [](proc& p){ p.stop(); } );
//     for_each(newps.begin(), newps.end(), [&procs](proc& p){
//             p.start();
//             procs.insert(procs.end(), p);
//         } );
// 
//     cout << "check " << path << " " << procs << endl;
//     return 0;
// }

static std::vector<proc>* fsit_[3];
static int fsit(const char *cpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    cout << "fsit " << cpath << endl;
    if (ftwbuf->level <= 2)
    {
        if (typeflag == FTW_F)
        {
            if (*cpath == '.')
                return 0;
            if (!(sb->st_mode & S_IXUSR))
                return 0;

            std::vector<proc>& procs = *fsit_[0];
            std::vector<proc>& newps = *fsit_[1];
            std::vector<proc>& oldps = *fsit_[2];

            auto i = find_if(oldps.begin(), oldps.end()
                    , [&cpath](proc& p){ return p.path()==cpath; });
            if (i == oldps.end()) {
                newps.push_back(proc(cpath)); // NEW
                // newps.insert(procs.end(), proc(cpath));
            } else {
                cout << *i << endl;
                procs.push_back(*i); // KEEP
                oldps.erase(i); // OLD
            }
        }
    }
    return 0;
}

int runs(const string& path, std::vector<proc>& procs)
{
    vector<proc> oldps(move(procs));
    vector<proc> newps;
    BOOST_ASSERT(empty(procs));

    fsit_[0] = &procs;
    fsit_[1] = &newps;
    fsit_[2] = &oldps;

    int ret = nftw(path.c_str(), fsit, 32, 0);;

    for(auto & p : oldps)
        p.stop();

    for(auto & p : newps)
    {
        p.start();
        procs.push_back(p);
    }

    cout << "runs " << path << " " << procs << endl;
    return ret;
}

string cwd(const string& dir)
{
    auto v = path_split(dir); // cout << v.size() << ":" << v << endl; exit(1);
    if (empty(v))
        exit(21);

    string d = v.back();
    v.pop_back();

    if (!empty(v))
    {
        string tmp = join(v, "/");
        if (chdir(tmp.c_str()) < 0)
            exit(22);
    }

    return d;
}

int lock_pidfile(const char* pidfile)
{
    int fd = open(pidfile, O_CREAT | O_SYNC | O_TRUNC | O_WRONLY);
    if (fd >= 0)
    {
        if (flock(fd, LOCK_EX|LOCK_NB) < 0)
        {
            close(fd);
            return -1;
        }

        string pid = lexical_cast<string>(getpid());
        write(fd, pid.data(), pid.size());
        // fflush(f); // fclose(f);
    }
    return fd;
}

static int sig_ = 0;
static void sig(int sig) { sig_ = sig; }


int main(int argc, char* const argv[])
{
    cout << getenv("PATH") << endl;
    if (argc == 1)
    {
        std::cerr << "Usage: a.out [path] [phones ...]" << std::endl;
        exit(1);
    }

    string path = cwd(argv[1]);

    if (!getenv("FOREGROUND"))
        daemon(1, 0);
    signal(SIGUSR1, sig);
    signal(SIGUSR2, sig);
    signal(SIGHUP, sig);
    signal(SIGCHLD, sig);
    signal(SIGTERM, sig);

    if (lock_pidfile("/tmp/wrunner.pid") < 0)
    {
        cerr << "pid file fail" << endl;
        exit(4);
    }

    std::vector<proc> procs;
    notify notify(make_iterator_range(&argv[2], &argv[argc]));

    while (1) // while (filesystem::exists(path))
    {
        int status;
        pid_t pid;
        while ( (pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            cout << "waited:" << pid << ":" << status << endl;
            if (kill(pid, SIGUSR1) < 0)
            {
                auto i = find_if(std::begin(procs),std::end(procs), [&pid](proc& p){return p.id()==pid;});
                if (i != procs.end())
                {
                    cout << "crash:" << *i << endl;
                    notify.add( i->path() );
                    procs.erase(i);
                }
            }
        }

        if (runs(path, procs) < 0)
            exit(5);
        cout << procs << endl;

        if (!notify.empty())
            notify.send();

        sleep(notify.empty() ? 90 : 3);

        if (sig_)
        {
            cout << "sig " << sig_ << endl;
            for (auto & p : procs)
                kill(p.id(), sig_);
            if (sig_ == SIGTERM)
                exit(0);
            sig_ = 0;
        }
    }

    return 0;
}

