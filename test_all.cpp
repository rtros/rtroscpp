#include "rtai_thread.h"

int main()
{
	//test thread
	rtai_thread::thread test_th;
	std::cout << test_th.get_id() << std::endl;
	std::cout << "thread pass!" << std::endl;

	//test mutex
	rtai_thread::mutex rtai_m;
	rtai_m.lock();
	rtai_m.unlock();
	std::cout << "mutex pass!" << std::endl;
	
	rtai_thread::shared_mutex rtai_sm;
	rtai_sm.lock();
	rtai_sm.unlock();
	std::cout << "shared_mutex pass!" << std::endl;

	//test scoped_lock
	{
		rtai_thread::mutex::scoped_lock rtai_sl;
	}
	std::cout << "mutex::scoped_lock pass!" << std::endl;

	{
		rtai_thread::recursive_mutex::scoped_lock rtai_rsl;
	}
	std::cout << "recursive_mutex::scoped_lock pass!" << std::endl;

	rtai_thread::condition_variable rtai_cv;
	
	return 0;
}
