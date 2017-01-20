#include <iostream>

#include <cstdint>
#include <array>
#include <queue>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>

class async_condition_variable : public boost::noncopyable
{
public:
	using signature = auto(boost::system::error_code) -> void;

	template <typename CompletionToken>
	using handler_type_t = typename boost::asio::handler_type<CompletionToken, signature>::type;
	
	template <typename CompletionToken>
	using async_result = typename boost::asio::async_result<handler_type_t<CompletionToken>>;
	
	template <typename CompletionToken>
	using async_result_t = typename async_result<CompletionToken>::type;
private:
	using handler_block = std::array<uint8_t, 128U>;
	
	class handler_holder_base : public boost::noncopyable
	{
	public:
		handler_holder_base() {}
		virtual ~handler_holder_base() {}
		 
		virtual void exec(boost::system::error_code ec) = 0;
	};
	
	template <typename CompletionToken>
	class handler_holder : public handler_holder_base
	{
	private:
		boost::asio::io_service& m_iosv;
	public:
		handler_type_t<CompletionToken> m_handler;
	
		handler_holder(boost::asio::io_service& iosv, handler_type_t<CompletionToken>&& handler)
			: m_iosv(iosv), m_handler(std::forward<handler_type_t<CompletionToken>&&>(handler)) {}
			
		void exec(boost::system::error_code ec)override
		{
			handler_type_t<CompletionToken> handler = std::move(m_handler);
			m_iosv.post([handler, ec](){ boost::asio::asio_handler_invoke(std::bind(handler, ec), &handler); });
		}
	};
private:
	boost::asio::io_service& m_iosv;
	std::queue<size_t> m_handlers;
	
	// handler storage
	std::vector<handler_block> m_storage;
	std::vector<size_t> m_freelist;
public:
	async_condition_variable(boost::asio::io_service& iosv)
		: m_iosv(iosv) {}

	~async_condition_variable()
	{
		while (!m_handlers.empty())
        {
			size_t idx = m_handlers.front();
			auto holder = reinterpret_cast<handler_holder_base*>(m_storage[idx].data());
			m_handlers.pop();
			try
			{
				holder->exec(boost::asio::error::make_error_code(boost::asio::error::interrupted));
			}
			catch (...)
			{
				free_handler(idx);
				continue;
			}
			free_handler(idx);
        }
		assert(m_freelist.size() == m_storage.size());
	}
private:
	template <typename CompletionToken>
	size_t alloc_handler(handler_type_t<CompletionToken>&& handler)
	{
		static_assert(sizeof(handler_holder<CompletionToken>) <= std::tuple_size<handler_block>::value, "handler is too bigger.");
		
		if (m_freelist.empty())
		{
			m_storage.emplace_back();
			auto& storage = m_storage.back();
			new(storage.data()) handler_holder<CompletionToken>(m_iosv, std::forward<handler_type_t<CompletionToken>&&>(handler));
			return m_storage.size() - 1;
		}
		else
		{
			size_t idx = m_freelist.back();
			auto& storage = m_storage[idx];
			m_freelist.pop_back();
			new(storage.data()) handler_holder<CompletionToken>(m_iosv, std::forward<handler_type_t<CompletionToken>&&>(handler));
			return idx;
		}
	}
	
	void free_handler(size_t idx)
	{
		auto& storage = m_storage[idx];
		auto holder = reinterpret_cast<handler_holder_base*>(m_storage[idx].data());
		holder->~handler_holder_base();
		m_freelist.push_back(idx);
	}
public:
	template <typename CompletionToken>
	async_result_t<CompletionToken> async_wait(CompletionToken&& token)
	{
		handler_type_t<CompletionToken> handler(std::forward<CompletionToken&&>(token));
		async_result<CompletionToken> result(handler);
		size_t idx = alloc_handler<CompletionToken>(std::move(handler));
		auto holder = reinterpret_cast<handler_holder<CompletionToken>*>(m_storage[idx].data());
		m_handlers.push(idx);
		return result.get();
	}

	void notify_one()
	{
		if (!m_handlers.empty())
        {
			size_t idx = m_handlers.front();
			auto holder = reinterpret_cast<handler_holder_base*>(m_storage[idx].data());
			m_handlers.pop();
			try
			{
				holder->exec(boost::system::error_code());
			}
			catch (...)
			{
				free_handler(idx);
				throw;
			}
			free_handler(idx);
        }
	}
	
	void notify_all()
	{
		while (!m_handlers.empty())
			notify_one();
	}
};

int main()
{
	boost::asio::io_service iosv;
	
	auto cond_var = std::make_shared<async_condition_variable>(iosv);
		
	boost::asio::spawn(iosv, [&](boost::asio::yield_context yield){
		boost::system::error_code ec;
		std::cout << "point 1.1" << std::endl; 
		cond_var->async_wait(yield);
		std::cout << "point 2.1" << std::endl;
	});
	
	boost::asio::spawn(iosv, [&](boost::asio::yield_context yield){
		std::cout << "point 3" << std::endl;
		cond_var->notify_all();
		std::cout << "point 4" << std::endl;
	});
	
	boost::asio::spawn(iosv, [&](boost::asio::yield_context yield){
		boost::system::error_code ec;
		std::cout << "point 1.2" << std::endl; 
		cond_var->async_wait(yield);
		std::cout << "point 2.2" << std::endl;
	});
	
	boost::asio::spawn(iosv, [&](boost::asio::yield_context yield){
		std::cout << "point 5" << std::endl;
		cond_var->notify_all();
		std::cout << "point 6" << std::endl;
	});
	
	iosv.run();
	return 0;
}
