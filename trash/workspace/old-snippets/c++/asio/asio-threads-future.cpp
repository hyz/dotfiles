/// http://boost.2283326.n4.nabble.com/ASIO-with-std-thread-td4645072.html
//
#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif
#include <cstdlib>
#include <cstddef>
#include <utility>
#include <iostream>
#include <exception>
#include <thread>
#include <future>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
namespace ma {
namespace test {
namespace asio_future {
class connect_task
{
public:
  typedef boost::system::error_code result;
  typedef std::future<result> future;
private:
  typedef connect_task this_type;
  typedef std::promise<result> promise;
  this_type& operator=(const this_type&);
public:
  connect_task()
    : promise_(std::make_shared<promise>())
  {
  }
  connect_task(const this_type& other)
    : promise_(other.promise_)
  {
  }
  connect_task(this_type&& other)
    : promise_(std::move(other.promise_))
  {
  }
  void operator()(const boost::system::error_code& error)
  {
    promise_->set_value(error);
  }
  future get_future()
  {
    return promise_->get_future();
  };
private:
  std::shared_ptr<promise> promise_;
}; // connect_task

class io_task
{
public:
  typedef std::pair<boost::system::error_code, std::size_t> result;
  typedef std::future<result> future;
private:
  typedef io_task this_type;
  typedef std::promise<result> promise;
  this_type& operator=(const this_type&);
public:
  io_task()
    : promise_(std::make_shared<promise>())
  {
  }
  io_task(const this_type& other)
    : promise_(other.promise_)
  {
  }
  io_task(this_type&& other)
    : promise_(std::move(other.promise_))
  {
  }
  void operator()(const boost::system::error_code& error,
      std::size_t bytes_transferred)
  {
    promise_->set_value(std::make_pair(error, bytes_transferred));
  }
  future get_future()
  {
    return promise_->get_future();
  };
private:
  std::shared_ptr<promise> promise_;
}; // io_task

class io_service_pool
{
private:
  typedef io_service_pool this_type;
  io_service_pool(const this_type&);
  this_type& operator=(const this_type&);
public:
  io_service_pool(boost::asio::io_service& io_service, std::size_t
size)
    : work_guard_(boost::in_place(std::ref(io_service)))
  {
    for (; size; --size)
    {
      threads_.emplace_back([&io_service]()
      {
        io_service.run();
      });
    }
  }

  ~io_service_pool()
  {
    work_guard_ = boost::none;
    std::for_each(threads_.begin(), threads_.end(), [](std::thread&
thread)
    {
      thread.join();
    });
  }
private:
  boost::optional<boost::asio::io_service::work> work_guard_;
  std::vector<std::thread> threads_;
}; // io_service_pool

void run_test()
{
  const boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::address_v4::loopback(), 7);
  const std::size_t cpu_count = std::thread::hardware_concurrency();
  const std::size_t io_threadpool_size = cpu_count < 2 ? 2 : cpu_count;
  boost::asio::io_service io_service(io_threadpool_size);
  io_service_pool io_threadpool(io_service, io_threadpool_size);

  boost::asio::ip::tcp::socket socket(io_service);
  {
    // Connect to localhost:7
    connect_task handler;
    connect_task::future future = handler.get_future();
    socket.async_connect(endpoint, std::move(handler));
    if (connect_task::result result = future.get())
    {
      std::cout << "Failed to connect: " << result << std::endl;
      return;
    }
    std::cout << "Connected" << std::endl;
  }

  {
    // Send some data
    const std::string data = "Hello from Boost.Asio!";
    io_task handler;
    io_task::future future = handler.get_future();
    boost::asio::async_write(
        socket, boost::asio::buffer(data), std::move(handler));
    io_task::result result = future.get();
    if (result.first)
    {
      std::cout << "Failed to send: " << result.first << std::endl;
      return;
    }
    std::cout << "Sent: " << data << std::endl;
  }

} // run_test
} // namespace asio_future
} // namespace test
} // namespace ma

#if defined(WIN32)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{
  try
  {
    ma::test::asio_future::run_test();
    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}

