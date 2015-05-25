#ifndef __PROXY_H__
#define __PROXY_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <string>
#include <arpa/inet.h>
#include <iostream>

#include "im_proxy.h"
#include "log.h"

class IM_Conn
{
    public:
        IM_Conn( boost::asio::io_service& io_serv, const std::string& h, const std::string& p )
            : timer_( io_serv), socket_( io_serv ), resolver_( io_serv )
        {
            host_ = h;
            port_ = p;
        }

        void send( const std::string& str )
        {
            if ( stage::error!=stage_ && stage::connecting!=stage_ ) {
                out_.push_back( str );
                send_();
            }
        }

        void start()
        {
            stage_ = stage::connecting;
            boost::asio::ip::tcp::resolver::query query( host_, port_ );
            resolver_.async_resolve( query, 
                    boost::bind( &IM_Conn::handle_resolve, this, boost::asio::placeholders::iterator,
                        boost::asio::placeholders::error ) );
        }


    private:
        struct stage { 
            enum type {
                error = 0,
                connecting = 1,
                idle,
                writing,
            };
        };

        stage::type stage_;
        std::string host_, port_;
        boost::asio::deadline_timer timer_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::ip::tcp::resolver resolver_;

        uint32_t len_;
        std::list< std::string > in_, out_;

    private:
        void handle_resolve( boost::asio::ip::tcp::resolver::iterator endpoint_iterator, 
                const boost::system::error_code& error )
        {
            if ( error ) {
                LOG_I << "Error: " << error.message(); //<<  std::endl;
                start_timer_connect();
                return;
            }

            boost::asio::async_connect( socket_, endpoint_iterator,
                    boost::bind( &IM_Conn::handle_connect, this, boost::asio::placeholders::error ) );
        }

        void handle_connect( const boost::system::error_code& error )
        {
            if ( error ) {
                LOG_I << "Error: " << error.message(); // << std::endl;
                start_timer_connect();
                return;
            }

            LOG_I<< "OK,connected......";
            stage_ = stage::idle;
            get_head();
            Proxy_Handle_Mgr::inst().hello();
        }

        void timed_connect( boost::system::error_code ec ) 
        {   
            if (ec) {
                LOG_I<< ec<< ec.message(); // <<std::endl;
                start_timer_connect();
                return;
            }   

            start();
        } 

        void restart_()
        {
            LOG_I<< "Warnning! Restarting.. "<< "stage:"<< stage_;
            if ( stage::connecting == stage_ ) { return; }
            start();
        } 

    private:
        void start_timer_connect()
        {
            timer_.expires_from_now( boost::posix_time::seconds( 5 ) );
            timer_.async_wait( boost::bind( &IM_Conn::timed_connect, this, boost::asio::placeholders::error ) );
        }

        void send_()
        {
            if ( stage::idle == stage_ && !out_.empty() ) {
                LOG_I<< "Sending "<< " size: "<< out_.size()<< " front:"<< out_.front();
                stage_ = stage::writing;
                
                boost::asio::async_write( socket_, 
                        boost::asio::buffer( out_.front() ), 
                        boost::bind( &IM_Conn::handle_send_, this, boost::asio::placeholders::error ) );
            }
        }

        void handle_send_( const boost::system::error_code& error )
        {
            if ( error ) { 
                handle_error( error );
                return;
            }

            out_.pop_front();
            stage_ = stage::idle;
            send_();
        }

        void get_head()
        {
            len_ = 0;
            boost::asio::async_read( socket_, 
                    boost::asio::buffer( &len_, sizeof( len_ ) ), 
                    boost::bind( &IM_Conn::get_body, this, boost::asio::placeholders::error ) );
        }

        void get_body( const boost::system::error_code& error )
        {
            if ( error ) {
                handle_error( error );
                return;
            }

            len_ = ntohl( len_ );
            in_.push_back( std::string( len_, 0 ) );

            boost::asio::async_read( socket_, 
                    boost::asio::buffer( const_cast<char*>( in_.back().c_str() ), len_ ), 
                    boost::bind( &IM_Conn::handle_get, this, boost::asio::placeholders::error ) );
        }

        void handle_get( const boost::system::error_code& error )
        {
            LOG_I<< "size:"<< in_.size() << " recieved:" << in_.back().size() << " content:" << in_.back();
            if ( error ) { 
                handle_error( error );
                return;
            }

            Proxy_Handle_Mgr::inst().dispatch( in_.front() );
            in_.pop_front();
            get_head();
        }

        void handle_error( const boost::system::error_code& error ) 
        {
            LOG_I<< "stage:"<< stage_<< error.message(); // << std::endl;

            if ( stage::error == stage_  ) { return; }
            stage_ = stage::error;
            boost::system::error_code err;
            socket_.shutdown( boost::asio::ip::tcp::socket::shutdown_both, err );
            if ( err ) {
                LOG_I << err.message();
            }
            socket_.close( err );
            if ( err ) {
                LOG_I << err.message();
            }

            in_.clear();
            out_.clear();
            sleep( 5 );

            restart_();
        }
};

#endif
