#ifndef RTAI_THREAD_HPP
#define RTAI_THREAD_HPP

#include "rtai_mutex.hpp"
#include "rtai_shared_mutex.hpp"
#include "rtai_condition_variable.hpp"
#include "rtai_pthread_mutex_scoped_lock.hpp"

#include "rtai_thread_data.hpp"
#include "rtai_thread_detail.hpp"

#include <boost/thread/lock_types.hpp>	// offer unique_lock & shared_lock
#include <boost/thread/tss.hpp>			// offer thread_specific_ptr

#endif