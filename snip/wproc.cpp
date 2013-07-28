#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <string>
#include <vector>
#include <algorithm>
#include <boost/range.hpp>
#include <boost/filesystem.hpp>

#include <boost/format.hpp>
#include <iostream>

#include "log.h"
#include "util.h"

using namespace boost;
using namespace std;
// using boost::asio::ip::tcp;

template <typename R> void notify(const R & r)
{
    typedef typename range_iterator<R>::type iterator;
    for (iterator i = std::begin(r); i != std::end(r); ++i)
    {
        sendsms(*i, "Warning: Server Core-Dump.");
    }
}

struct proc
{
    filesystem::path path_;
    pid_t pid_;

    explicit proc(const filesystem::path& p) : path_(p) { pid_ = 0; }
    proc() { pid_ = 0; }

    pid_t start();
    void  stop();

    pid_t id() const { return pid_; }
    const filesystem::path& path() const { return path_; }
};

pid_t proc::start()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        // LOG
        return 0;
    }

    if (pid == 0)
    {
        execl(path_.c_str(), path_.c_str(), 0);
        BOOST_ASSERT(false);
    }
    else // (pid > 0)
    {
        this->pid_ = pid;
    }

    return pid;
}

void proc::stop()
{
    // man 7 signal
    if (pid_ > 0)
    {
        kill(pid_, SIGTERM);
        kill(pid_, SIGKILL);
        pid_ = 0;
    }
}

std::vector<proc>& list_procs(std::vector<proc>& procs, const filesystem::path& path)
{
    if (filesystem::is_directory(path))
    {
        for_each(filesystem::directory_iterator(path), filesystem::directory_iterator(),
                [&procs](boost::filesystem::directory_entry& it) {
                    list_procs(procs, it.path());
                });
    }
    else if (filesystem::is_regular(path)) // filesystem::is_symlink(path) 
    {
        auto i = find_if(procs.begin(), procs.end(),
                [&path](proc& p){ return p.path()==path; });
        if (i == procs.end()) {
            i = procs.insert(procs.end(), proc(path));
            i->start();
        // } else {
        //     i->stop();
        //     procs.erase(i, procs.end());
        }
    }

    return procs;
}

// const char* phones[] = { "13798378429", "13798378429" };
int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        std::cerr << "Usage: a.out [path] [phones ...]" << std::endl;
        exit(1);
    }

    filesystem::path path = filesystem::system_complete(argv[1]);
    if (!filesystem::exists(path))
    {
        exit(2);
    }

    daemon(0, 0);

    std::vector<proc> procs;

    while (1)
    {
        while (procs.empty())
        {
            list_procs(procs, path);
            if (procs.empty())
                sleep(9);
        }

        int status;
        pid_t pid = wait(&status);
        // LOG

        notify( make_iterator_range(&argv[1], &argv[argc]) );
        sleep(1);

        for_each(procs.begin(), procs.end(),
                [&pid](proc& p) {
                    if (p.id() == pid) {
                        p.stop();
                        p.start();
                    }
                });
    }

    return 0;
}

