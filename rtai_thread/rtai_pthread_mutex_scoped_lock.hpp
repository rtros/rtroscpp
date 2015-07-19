#ifndef RTAI_PTHREAD_MUTEX_SCOPED_LOCK_HPP
#define RTAI_PTHREAD_MUTEX_SCOPED_LOCK_HPP

#include <rtai_posix.h>
#include <boost/assert.hpp>
#include <boost/config/abi_prefix.hpp>

namespace rtai_thread
{
    class pthread_mutex_scoped_lock
    {
        pthread_mutex_t* m;
        bool locked;
    public:
        explicit pthread_mutex_scoped_lock(pthread_mutex_t* m_):
            m(m_),locked(true)
        {
            BOOST_VERIFY(!pthread_mutex_lock(m));
        }
        void unlock()
        {
            BOOST_VERIFY(!pthread_mutex_unlock(m));
            locked=false;
        }
        
        ~pthread_mutex_scoped_lock()
        {
            if(locked)
            {
                unlock();
            }
        }
        
    };

    class pthread_mutex_scoped_unlock
    {
        pthread_mutex_t* m;
    public:
        explicit pthread_mutex_scoped_unlock(pthread_mutex_t* m_):
            m(m_)
        {
            BOOST_VERIFY(!pthread_mutex_unlock(m));
        }
        ~pthread_mutex_scoped_unlock()
        {
            BOOST_VERIFY(!pthread_mutex_lock(m));
        }
        
    };
}

#include <boost/config/abi_suffix.hpp>

#endif
