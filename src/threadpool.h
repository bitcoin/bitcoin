#include <memory>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "util.h"
struct thread_pool {
	typedef std::unique_ptr<boost::asio::io_service::work> asio_worker;

	thread_pool(int threads) :service(), service_worker(new asio_worker::element_type(service)) {
		for (int i = 0; i < threads; ++i) {
			auto worker = [this] { return service.run(); };
			boost::thread_t newThread = boost::thread(worker);
			grp.add_thread(newThread);
			ScheduleBatchPriority(newThread.native_handle());
		}
	}

	template<class F>
	void enqueue(F f) {
		service.post(f);
	}

	~thread_pool() {
		service_worker.reset();
		grp.join_all();
		service.stop();
	}

private:
	boost::asio::io_service service;
	asio_worker service_worker;
	boost::thread_group grp;
};