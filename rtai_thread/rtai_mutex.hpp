#ifndef RTAI_MUTEX_HPP
#define RTAI_MUTEX_HPP

#include "rtai_posix.h"
#include "rtai_pthread_mutex_scoped_lock.hpp"
#include <boost/thread/detail/config.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/detail/delete.hpp>
#if defined BOOST_THREAD_PROVIDES_NESTED_LOCKS
#include <boost/thread/lock_types.hpp>
#endif
#include <boost/thread/thread_time.hpp>
#include <boost/thread/xtime.hpp>
#include <errno.h>
#include "rtai_timespec.hpp"
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>

#if (defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS-0)>=200112L) \
 || (defined(__ANDROID__) && defined(__ANDROID_API__) && __ANDROID_API__ >= 21)
#ifndef BOOST_PTHREAD_HAS_TIMEDLOCK
#define BOOST_PTHREAD_HAS_TIMEDLOCK
#endif
#endif


#include <boost/config/abi_prefix.hpp>

#ifndef BOOST_THREAD_HAS_NO_EINTR_BUG
#define BOOST_THREAD_HAS_EINTR_BUG
#endif

namespace rtai_thread
{

#define __KERNEL__	// use rtai_posix pthread functions

  namespace posix {
#ifdef BOOST_THREAD_HAS_EINTR_BUG
    inline int pthread_mutex_destroy(pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_mutex_destroy(m);
      } while (ret == EINTR);
      return ret;
    }
    inline int pthread_mutex_lock(pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_mutex_lock(m);
      } while (ret == EINTR);
      return ret;
    }
    inline int pthread_mutex_unlock(pthread_mutex_t* m)
    {
      int ret;
      do
      {
          ret = ::pthread_mutex_unlock(m);
      } while (ret == EINTR);
      return ret;
    }
#else
    inline int pthread_mutex_destroy(pthread_mutex_t* m)
    {
      return ::pthread_mutex_destroy(m);
    }
    inline int pthread_mutex_lock(pthread_mutex_t* m)
    {
      return ::pthread_mutex_lock(m);
    }
    inline int pthread_mutex_unlock(pthread_mutex_t* m)
    {
      return ::pthread_mutex_unlock(m);
    }

#endif

  }

	class mutex
	{
	private:
		pthread_mutex_t m;
	public:
		BOOST_THREAD_NO_COPYABLE(mutex)

		mutex()
        {
            int const res=pthread_mutex_init(&m,NULL);
            if(res)
            {
                boost::throw_exception(boost::thread_resource_error(res, "rtai_thread:: mutex constructor failed in pthread_mutex_init_rt"));
            }
        }
		~mutex()
		{
			int const res = posix::pthread_mutex_destroy(&m);
          	boost::ignore_unused(res);
          	BOOST_ASSERT(!res);
		}

		void lock(){
            int res = posix::pthread_mutex_lock(&m);
            if (res)
            {
                boost::throw_exception(boost::lock_error(res,"rtai_thread:: mutex lock failed in pthread_mutex_lock"));
            }
		}

		void unlock(){
            int res = posix::pthread_mutex_unlock(&m);
            (void)res;
            BOOST_ASSERT(res == 0);
		}
	
        bool try_lock()
        {
            int res;
            do
            {
                res = pthread_mutex_trylock(&m);
            } while (res == EINTR);
            if (res== -EBUSY)	// 由于rtat_posix.h返回值为-EBUSY，这里修改了符号
            {
                return false;
            }

            return !res;
        }

        typedef boost::unique_lock<mutex> scoped_lock;
        typedef boost::detail::try_lock_wrapper<mutex> scoped_try_lock;
	};

	typedef mutex try_mutex;
	
	class recursive_mutex
    {
    private:
        pthread_mutex_t m;
#ifndef BOOST_THREAD_HAS_PTHREAD_MUTEXATTR_SETTYPE
        pthread_cond_t cond;
        bool is_locked;
        pthread_t owner;
        unsigned count;
#endif
    public:
        BOOST_THREAD_NO_COPYABLE(recursive_mutex)
        recursive_mutex()
        {
#ifdef BOOST_THREAD_HAS_PTHREAD_MUTEXATTR_SETTYPE
            pthread_mutexattr_t attr;

            int const init_attr_res=pthread_mutexattr_init(&attr);
            if(init_attr_res)
            {
                boost::throw_exception(thread_resource_error(init_attr_res, "rtai_thread:: recursive_mutex constructor failed in pthread_mutexattr_init"));
            }
            int const set_attr_res=pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
            if(set_attr_res)
            {
                BOOST_VERIFY(!pthread_mutexattr_destroy(&attr));
                boost::throw_exception(thread_resource_error(set_attr_res, "rtai_thread:: recursive_mutex constructor failed in pthread_mutexattr_settype"));
            }

            int const res=pthread_mutex_init(&m,&attr);
            if(res)
            {
                BOOST_VERIFY(!pthread_mutexattr_destroy(&attr));
                boost::throw_exception(thread_resource_error(res, "rtai_thread:: recursive_mutex constructor failed in pthread_mutex_init"));
            }
            BOOST_VERIFY(!pthread_mutexattr_destroy(&attr));
#else
            int const res=pthread_mutex_init(&m,NULL);
            if(res)
            {
                boost::throw_exception(boost::thread_resource_error(res, "rtai_thread:: recursive_mutex constructor failed in pthread_mutex_init"));
            }
            int const res2=pthread_cond_init(&cond,NULL);
            if(res2)
            {
                BOOST_VERIFY(!pthread_mutex_destroy(&m));
                boost::throw_exception(boost::thread_resource_error(res2, "rtai_thread:: recursive_mutex constructor failed in pthread_cond_init"));
            }
            is_locked=false;
            count=0;
#endif
        }
        ~recursive_mutex()
        {
            BOOST_VERIFY(!pthread_mutex_destroy(&m));
#ifndef BOOST_THREAD_HAS_PTHREAD_MUTEXATTR_SETTYPE
            BOOST_VERIFY(!pthread_cond_destroy(&cond));
#endif
        }

#ifdef BOOST_THREAD_HAS_PTHREAD_MUTEXATTR_SETTYPE
        void lock()
        {
            BOOST_VERIFY(!pthread_mutex_lock(&m));
        }

        void unlock()
        {
            BOOST_VERIFY(!pthread_mutex_unlock(&m));
        }

        bool try_lock() BOOST_NOEXCEPT
        {
            int const res=pthread_mutex_trylock(&m);
            BOOST_ASSERT(!res || res== -EBUSY);	// 由于rtat_posix.h返回值为-EBUSY，这里修改了符号
            return !res;
        }
#define BOOST_THREAD_DEFINES_RECURSIVE_MUTEX_NATIVE_HANDLE
        typedef pthread_mutex_t* native_handle_type;
        native_handle_type native_handle()
        {
            return &m;
        }

#else
        void lock()
        {
            rtai_thread::pthread_mutex_scoped_lock const local_lock(&m);
            if(is_locked && pthread_equal(owner,pthread_self()))
            {
                ++count;
                return;
            }

            while(is_locked)
            {
                BOOST_VERIFY(!pthread_cond_wait(&cond,&m));
            }
            is_locked=true;
            ++count;
            owner=pthread_self();   //pthread_self?
        }

        void unlock()
        {
            rtai_thread::pthread_mutex_scoped_lock const local_lock(&m);
            if(!--count)
            {
                is_locked=false;
            }
            BOOST_VERIFY(!pthread_cond_signal(&cond));
        }

        bool try_lock()
        {
            rtai_thread::pthread_mutex_scoped_lock const local_lock(&m);
            if(is_locked && !pthread_equal(owner,pthread_self()))
            {
                return false;
            }
            is_locked=true;
            ++count;
            owner=pthread_self();
            return true;
        }

#endif

#if defined BOOST_THREAD_PROVIDES_NESTED_LOCKS
        typedef boost::unique_lock<recursive_mutex> scoped_lock;
        typedef boost::detail::try_lock_wrapper<recursive_mutex> scoped_try_lock;
#endif
    };

    typedef recursive_mutex recursive_try_mutex;
}

#include <boost/config/abi_suffix.hpp>
#endif