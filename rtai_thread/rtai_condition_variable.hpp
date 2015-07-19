#ifndef RTAI_CONDITION_VARIABLE_HPP
#define RTAI_CONDITION_VARIABLE_HPP

#include "rtai_timespec.hpp"
#include "rtai_pthread_mutex_scoped_lock.hpp"
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
#include "rtai_thread_data.hpp"
#endif
#include "rtai_condition_variable.hpp"
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>

#include <boost/config/abi_prefix.hpp>

namespace rtai_thread
{
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
    namespace this_thread
    {
        void BOOST_THREAD_DECL interruption_point();
    }
#endif

    namespace thread_cv_detail
    {
        template<typename MutexType>
        struct lock_on_exit
        {
            MutexType* m;

            lock_on_exit():
                m(0)
            {}

            void activate(MutexType& m_)
            {
                m_.unlock();
                m=&m_;
            }
            ~lock_on_exit()
            {
                if(m)
                {
                    m->lock();
                }
           }
        };
    }

    inline void condition_variable::wait(boost::unique_lock<mutex>& m)
    {
#if defined BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED
        if(! m.owns_lock())
        {
            boost::throw_exception(condition_error(-1, "boost::condition_variable::wait() failed precondition mutex not owned"));
        }
#endif
        int res=0;
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            thread_cv_detail::lock_on_exit<boost::unique_lock<mutex> > guard;
            detail::interruption_checker check_for_interruption(&internal_mutex,&cond);
            guard.activate(m);
            do {
              res = pthread_cond_wait(&cond,&internal_mutex);
            } while (res == EINTR);
#else
            //rtai_thread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
            pthread_mutex_t* the_mutex = m.mutex()->native_handle();
            do {
              res = pthread_cond_wait(&cond,the_mutex);
            } while (res == EINTR);
#endif
        }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        this_thread::interruption_point();
#endif
        if(res)
        {
            boost::throw_exception(boost::condition_error(res, "boost::condition_variable::wait failed in pthread_cond_wait"));
        }
    }

    inline bool condition_variable::do_wait_until(
                boost::unique_lock<mutex>& m,
                struct timespec const &timeout)
    {
#if defined BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED
        if (!m.owns_lock())
        {
            boost::throw_exception(condition_error(EPERM, "boost::condition_variable::do_wait_until() failed precondition mutex not owned"));
        }
#endif
        thread_cv_detail::lock_on_exit<boost::unique_lock<mutex> > guard;
        int cond_res;
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            detail::interruption_checker check_for_interruption(&internal_mutex,&cond);
            guard.activate(m);
            cond_res=pthread_cond_timedwait(&cond,&internal_mutex,&timeout);
#else
            //rtai_thread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
            pthread_mutex_t* the_mutex = m.mutex()->native_handle();
            cond_res=pthread_cond_timedwait(&cond,the_mutex,&timeout);
#endif
        }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        this_thread::interruption_point();
#endif
        if(cond_res==ETIMEDOUT)
        {
            return false;
        }
        if(cond_res)
        {
            boost::throw_exception(boost::condition_error(cond_res, "boost::condition_variable::do_wait_until failed in pthread_cond_timedwait"));
        }
        return true;
    }

    inline void condition_variable::notify_one() BOOST_NOEXCEPT
    {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        rtai_thread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
#endif
        BOOST_VERIFY(!pthread_cond_signal(&cond));
    }

    inline void condition_variable::notify_all() BOOST_NOEXCEPT
    {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        rtai_thread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
#endif
        BOOST_VERIFY(!pthread_cond_broadcast(&cond));
    }

}

#include <boost/config/abi_suffix.hpp>

#endif