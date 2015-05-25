// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <list>
#include <fstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <curl/curl.h>

// int count = 0;
boost::mutex mutex;
struct request_info
{
    std::string url;
    std::string cook;
    std::string post;
};
std::list<request_info> requests_;

// void increment_count()
// {
//     boost::unique_lock<boost::mutex> lock(mutex);
// 
//     std::cout << "count = " << ++count << std::endl;
// }
// 
// boost::thread_group threads2;
// boost::thread* th2 = 0;
// 
// void increment_count_2()
// {
//     boost::unique_lock<boost::mutex> lock(mutex);
//     BOOST_TEST(threads2.is_this_thread_in());
//     std::cout << "count = " << ++count << std::endl;
// }

size_t discard(char *ptr, size_t size, size_t nmemb, void *hf)
{
    return nmemb;
}

//bool read_req(std::string & url, std::string & cont)
//{
//    boost::unique_lock<boost::mutex> lock(mutex);
//    if (requests_.empty())
//        return 0;
//
//    url = requests_.front();
//    requests_.pop_front();
//    return 1;
//}
//
//void loop_http_get()
//{
//    std::string url, cont;
//    while (read_req(url, cont))
//    {
//        int ec = 0;
//        do {
//            CURL* curl;
//            if ( (curl = curl_easy_init()) == 0)
//            {
//                usleep(500);
//                continue;
//            }
//
//            // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
//            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//            // -- CURLOPT_POST
//
//            //curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, fprog_);
//            //curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, aself);
//            //curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
//            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
//         
//            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard);
//            // curl_easy_setopt(curl, CURLOPT_WRITEDATA, hf);
//
//            ec = curl_easy_perform(curl);
//            if (ec)
//                ;
//
//            curl_easy_cleanup(curl);
//        } while (ec);
//    }
//}

bool take_request(request_info& req)//(std::string & url, std::string& cook, std::string & cont)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    if (requests_.empty())
        return 0;
    std::swap(requests_.front(), req);
    requests_.pop_front();
    return 1;
}

void loop_request()
{
    request_info req;
    while (take_request(req))
    {
        int ec = 0;
        do {
            CURL* curl;
            // std::cout << url <<" "<< cook <<" "<< cont << "\n";
            if ( (curl = curl_easy_init()) == 0)
            {
                usleep(500);
                continue;
            }

            // shared_ptr<curl_slist> headers;
            // curl_set_httpheader(headers, k, val);
            // void curl_set_httpheader(shared_ptr<curl_slist> headers, std::string const & k, std::string const & val)
            // {
            //     std::string h = k + ": " + val;
            //     curl_slist* l = curl_slist_append(headers.get(), h.c_str());
            //     if (l != headers.get())
            //         headers = shared_ptr<curl_slist>(l, curl_slist_free_all);
            // }

            // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_URL, req.url.c_str());
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard);
            // curl_easy_setopt(curl, CURLOPT_WRITEDATA, hf);

            if (!req.cook.empty())
            {
                curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
                curl_easy_setopt(curl, CURLOPT_COOKIE, req.cook.c_str());
            }

            // curl_easy_setopt(p.curl, CURLOPT_HTTPHEADER, headers.get());
            // curl_easy_setopt(curl, CURLOPT_POST, 1L);
            // curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)cont.size());
            if (!req.post.empty())
            {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.post.c_str());
            }

            ec = curl_easy_perform(curl);
            if (ec)
                ;

            curl_easy_cleanup(curl);
        } while (ec);
    }
}

size_t init_reqlist(std::list<request_info>& reqs, const char* srcfn)
{
    size_t bytes = 0;
    std::ifstream infile(srcfn);
    std::string line;
    while (getline(infile, line))
    {
        std::istringstream ins(line);
        request_info req;
        if (!getline(ins, req.url,'\t'))
        {
            std::cerr << "bad line: " << line;
            continue;
        }
        if (getline(ins, req.cook,'\t'))
            if (getline(ins, req.post, '\t'))
                bytes += req.post.size();
        reqs.push_back(req);
    }
    return bytes;
}

int main(int argc, char *const argv[])
{
    if (argc < 3)
        exit(1);
    size_t post_bytes = init_reqlist(requests_, argv[2]);
    size_t n_request = requests_.size();
    time_t t_start = 0;

    boost::thread_group threads;

    int n_thread = boost::lexical_cast<int>(argv[1]);
    if (n_thread > 0)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        for (int i = 0; i < n_thread; ++i)
            threads.create_thread(&loop_request);
        std::cout << n_thread << " threads created.\n";
        t_start = time(0);
    }

    threads.join_all();
    time_t t_end = time(0);

    std::cout << "done.\n";
    std::cout << "total requests: " << n_request <<"\n";
    std::cout << "total seconds used: " << t_end - t_start <<"\n";
    if (post_bytes > 0)
        std::cout << "total bytes posted: " << post_bytes <<"\n";

//  {
//    boost::thread_group threads;
//    for (int i = 0; i < 3; ++i)
//        threads.create_thread(&increment_count);
//    threads.join_all();
//  }
// #if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
//   {
//     boost::thread_group threads;
//     for (int i = 0; i < 3; ++i)
//         threads.create_thread(&increment_count);
//     threads.interrupt_all();
//     threads.join_all();
//   }
// #endif
//   {
//     boost::thread_group threads;
//     boost::thread* th = new boost::thread(&increment_count);
//     threads.add_thread(th);
//     BOOST_TEST(! threads.is_this_thread_in());
//     threads.join_all();
//   }
//   {
//     boost::thread_group threads;
//     boost::thread* th = new boost::thread(&increment_count);
//     threads.add_thread(th);
//     BOOST_TEST(threads.is_thread_in(th));
//     threads.remove_thread(th);
//     BOOST_TEST(! threads.is_thread_in(th));
//     th->join();
//   }
//   {
//     {
//       boost::unique_lock<boost::mutex> lock(mutex);
//       boost::thread* th2 = new boost::thread(&increment_count_2);
//       threads2.add_thread(th2);
//     }
//     threads2.join_all();
//   }
  return boost::report_errors();
}
