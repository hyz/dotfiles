#ifndef SINGLETON_H__
#define SINGLETON_H__

#include <boost/noncopyable.hpp>
#include <boost/assert.hpp>

template <class T>
struct singleton : boost::noncopyable
{
    static T& instance()
    {
        BOOST_ASSERT(thiz);
        return *thiz;
    }

protected:
    singleton()
    {
        BOOST_ASSERT(!thiz);
        thiz = static_cast<T*>(this);
    }

    static T* thiz;
};
template<class T> T* singleton<T>::thiz = 0;

#endif // SINGLETON_H__

