#ifndef RTAI_CONDITION_VARIABLE_FWD_HPP
#define RTAI_CONDITION_VARIABLE_FWD_HPP

#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <rtai_posix.h>
#include <boost/thread/cv_status.hpp>
#include "rtai_mutex.hpp"
#include <boost/thread/lock_types.hpp>
#include <boost/thread/thread_time.hpp>
#include "rtai_timespec.hpp"
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <boost/config/abi_prefix.hpp>

namespace rtai_thread
{
	class condition_variable
    {
    private:
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        pthread_mutex_t internal_mutex;
#endif
        pthread_cond_t cond;

    public:
    //private: // used by boost::thread::try_join_until

        inline bool do_wait_until(
            boost::unique_lock<mutex>& lock,
            struct timespec const &timeout);

        bool do_wait_for(
            boost::unique_lock<mutex>& lock,
            struct timespec const &timeout)
        {
          return do_wait_until(lock, boost::detail::timespec_plus(timeout, boost::detail::timespec_now()));
        }

    public:
      BOOST_THREAD_NO_COPYABLE(condition_variable)
        condition_variable()
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            int const res=pthread_mutex_init(&internal_mutex,NULL);
            if(res)
            {
                boost::throw_exception(boost::thread_resource_error(res, "rtai_thread::condition_variable::condition_variable() constructor failed in pthread_mutex_init"));
            }
#endif
            int const res2=pthread_cond_init(&cond,NULL);
            if(res2)
            {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
#endif
                boost::throw_exception(boost::thread_resource_error(res2, "rtai_thread::condition_variable::condition_variable() constructor failed in pthread_cond_init"));
            }
        }
        ~condition_variable()
        {
            int ret;
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            do {
              ret = pthread_mutex_destroy(&internal_mutex);
            } while (ret == EINTR);
            BOOST_ASSERT(!ret);
#endif
            do {
              ret = pthread_cond_destroy(&cond);
            } while (ret == EINTR);
            BOOST_ASSERT(!ret);
        }

        void wait(boost::unique_lock<mutex>& m);

        template<typename predicate_type>
        void wait(boost::unique_lock<mutex>& m,predicate_type pred)
        {
            while(!pred()) wait(m);
        }


#if defined BOOST_THREAD_USES_DATETIME
        inline bool timed_wait(
            boost::unique_lock<mutex>& m,
            boost::system_time const& abs_time)
        {
#if defined BOOST_THREAD_WAIT_BUG
            struct timespec const timeout=boost::detail::to_timespec(abs_time + BOOST_THREAD_WAIT_BUG);
            return do_wait_until(m, timeout);
#else
            struct timespec const timeout=boost::detail::to_timespec(abs_time);
            return do_wait_until(m, timeout);
#endif
        }
        bool timed_wait(
            boost::unique_lock<mutex>& m,
            boost::xtime const& abs_time)
        {
            return timed_wait(m,boost::system_time(abs_time));
        }

        template<typename duration_type>
        bool timed_wait(
            boost::unique_lock<mutex>& m,
            duration_type const& wait_duration)
        {
            return timed_wait(m, boost::get_system_time()+wait_duration);
        }

        template<typename predicate_type>
        bool timed_wait(
            boost::unique_lock<mutex>& m,
            boost::system_time const& abs_time,predicate_type pred)
        {
            while (!pred())
            {
                if(!timed_wait(m, abs_time))
                    return pred();
            }
            return true;
        }

        template<typename predicate_type>
        bool timed_wait(
            boost::unique_lock<mutex>& m,
            boost::xtime const& abs_time,predicate_type pred)
        {
            return timed_wait(m, boost::system_time(abs_time),pred);
        }

        template<typename duration_type,typename predicate_type>
        bool timed_wait(
            boost::unique_lock<mutex>& m,
            duration_type const& wait_duration,predicate_type pred)
        {
            return timed_wait(m, boost::get_system_time()+wait_duration,pred);
        }
#endif

#ifdef BOOST_THREAD_USES_CHRONO

        template <class Duration>
        boost::cv_status
        wait_until(
                boost::unique_lock<mutex>& lock,
                const boost::chrono::time_point<boost::chrono::system_clock, Duration>& t)
        {
          using namespace boost::chrono;
          typedef time_point<system_clock, nanoseconds> nano_sys_tmpt;
          wait_until(lock,
                        nano_sys_tmpt(ceil<nanoseconds>(t.time_since_epoch())));
          return system_clock::now() < t ? boost::cv_status::no_timeout :
                                             boost::cv_status::timeout;
        }

        template <class Clock, class Duration>
        boost::cv_status
        wait_until(
                boost::unique_lock<mutex>& lock,
                const boost::chrono::time_point<Clock, Duration>& t)
        {
          using namespace boost::chrono;
          system_clock::time_point     s_now = system_clock::now();
          typename Clock::time_point  c_now = Clock::now();
          wait_until(lock, s_now + ceil<nanoseconds>(t - c_now));
          return Clock::now() < t ? boost::cv_status::no_timeout : boost::cv_status::timeout;
        }

        template <class Clock, class Duration, class Predicate>
        bool
        wait_until(
                boost::unique_lock<mutex>& lock,
                const boost::chrono::time_point<Clock, Duration>& t,
                Predicate pred)
        {
            while (!pred())
            {
                if (wait_until(lock, t) == boost::cv_status::timeout)
                    return pred();
            }
            return true;
        }


        template <class Rep, class Period>
        boost::cv_status
        wait_for(
                boost::unique_lock<mutex>& lock,
                const boost::chrono::duration<Rep, Period>& d)
        {
          using namespace boost::chrono;
          system_clock::time_point s_now = system_clock::now();
          steady_clock::time_point c_now = steady_clock::now();
          wait_until(lock, s_now + ceil<nanoseconds>(d));
          return steady_clock::now() - c_now < d ? boost::cv_status::no_timeout :
                                                   boost::cv_status::timeout;

        }


        template <class Rep, class Period, class Predicate>
        bool
        wait_for(
                boost::unique_lock<mutex>& lock,
                const boost::chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
          return wait_until(lock, boost::chrono::steady_clock::now() + d, boost::move(pred));
        }
#endif

#define BOOST_THREAD_DEFINES_CONDITION_VARIABLE_NATIVE_HANDLE
        typedef pthread_cond_t* native_handle_type;
        native_handle_type native_handle()
        {
            return &cond;
        }

        void notify_one() BOOST_NOEXCEPT;
        void notify_all() BOOST_NOEXCEPT;

#ifdef BOOST_THREAD_USES_CHRONO
        inline boost::cv_status wait_until(
            boost::unique_lock<mutex>& lk,
            boost::chrono::time_point<boost::chrono::system_clock, boost::chrono::nanoseconds> tp)
        {
            using namespace boost::chrono;
            nanoseconds d = tp.time_since_epoch();
            timespec ts = boost::detail::to_timespec(d);
            if (do_wait_until(lk, ts)) return boost::cv_status::no_timeout;
            else return boost::cv_status::timeout;
        }
#endif
    };

    BOOST_THREAD_DECL void notify_all_at_thread_exit(condition_variable& cond, boost::unique_lock<mutex> lk);

}

#include <boost/config/abi_suffix.hpp>
#endif