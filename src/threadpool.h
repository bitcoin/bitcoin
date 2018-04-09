#include <memory>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <vector>
#include "util.h"
struct thread_pool {
	thread_pool(int threads) : next_io_service_(0) {
		// Create a pool of threads to run all of the io_services.
		
		for (int i = 0; i < threads; ++i) {
			io_service_ptr io_service(new boost::asio::io_service);
			work_ptr work(new boost::asio::io_service::work(*io_service));
			io_services_.push_back(io_service);
			work_.push_back(work);

			boost::shared_ptr<boost::thread> thread(new boost::thread(
				boost::bind(&boost::asio::io_service::run, io_service)));
			threads_.push_back(thread);

			ScheduleBatchPriority(thread->native_handle());
		}
	}

	template<class F>
	void enqueue(F f) {
		get_io_service().post(f);
	}

	~thread_pool() {
		// Explicitly stop all io_services.
		for (std::size_t i = 0; i <work_.size(); ++i)
			work_[i]->reset();
		
		// Wait for all threads in the pool to exit.
		for (std::size_t i = 0; i < threads_.size(); ++i)
			threads_[i]->join();

		// Explicitly stop all io_services.
		for (std::size_t i = 0; i < io_services_.size(); ++i)
			io_services_[i]->stop();
		
	}
	boost::asio::io_service& io_service_pool::get_io_service()
	{
		// Use a round-robin scheme to choose the next io_service to use.
		boost::asio::io_service& io_service = *io_services_[next_io_service_];
		++next_io_service_;
		if (next_io_service_ == io_services_.size())
			next_io_service_ = 0;
		return io_service;
	}
private:
	typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;
	typedef boost::shared_ptr<boost::thread> thread_ptr;

	// Create a pool of threads to run all of the io_services.
	std::vector<thread_ptr> threads_;

	/// The pool of io_services.
	std::vector<io_service_ptr> io_services_;

	/// The work that keeps the io_services running.
	std::vector<work_ptr> work_;

	/// The next io_service to use for a connection.
	std::size_t next_io_service_;
};