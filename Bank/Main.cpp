#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <chrono>
#include <random>
#include <deque>
#include <functional>
#include <stdlib.h>

struct ATM
{
	std::thread thread;
	std::chrono::time_point<std::chrono::high_resolution_clock> work_start = std::chrono::high_resolution_clock::now();
	std::chrono::milliseconds work_time_estimate = std::chrono::milliseconds(1);

	ATM(std::deque<int>& queue,
		std::mutex& mutex,
		std::condition_variable& cv_atms,
		std::condition_variable& cv_gen,
		int id)

		: thread([this, &queue, &mutex, &cv_atms, &cv_gen, id]()
			{
				std::default_random_engine r_eng(id);
				std::uniform_int_distribution<> sleep_time_range(2400, 3000);

				while (true)
				{
					std::unique_lock<std::mutex> lock(mutex);
					cv_atms.wait(lock, [&queue]()
						{
							return !queue.empty();
						});

					int task = queue.front();
					queue.pop_front();

					int work_time = sleep_time_range(r_eng);
					work_time_estimate = std::chrono::milliseconds(work_time);
					work_start = std::chrono::high_resolution_clock::now();

					lock.unlock();

					cv_gen.notify_one();

					// Process task
					
					std::this_thread::sleep_for(work_time_estimate);
				}
			})
	{
	}

};

int main()
{
	int n_atms = 0;
	int queue_size = 0;

	std::cin >> n_atms;
	std::cin >> queue_size;

	std::deque<int> queue;
	std::mutex mutex;
	std::condition_variable cvar_atms;
	std::condition_variable cvar_gen;

	std::vector<ATM> atms;
	atms.reserve(n_atms);

	for (int i = 0; i < n_atms; i++)
	{
		atms.emplace_back(queue, mutex, cvar_atms, cvar_gen, i * n_atms);
	}

	std::default_random_engine r_eng(time(nullptr));
	std::uniform_int_distribution<> sleep_time_range(400, 600);

	int task_counter = 0;

	while (true)
	{
		int sleep_time = sleep_time_range(r_eng);
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));

		std::unique_lock<std::mutex> lock(mutex);
		cvar_gen.wait(lock, [&queue, queue_size]()
			{
				return queue.size() != queue_size;
			});
		queue.push_back(task_counter++);
		auto sz = queue.size();

		auto now = std::chrono::high_resolution_clock::now();
		std::cout << "=====================" << std::endl;
		for (int i = 0; i < n_atms; i++)
		{
			using ms = std::chrono::milliseconds;
			auto work_time = std::chrono::duration_cast<ms>(now - atms[i].work_start);
			bool does_sleep = work_time > atms[i].work_time_estimate;
			if (does_sleep)
				std::cout << "ATM#" << i << ": " << "Sleeping..." << std::endl;
			else
				std::cout << "ATM#" << i << ": " << work_time.count() << "/" << atms[i].work_time_estimate.count() << std::endl;
		}
		lock.unlock();
		cvar_atms.notify_all();

		std::cout << "=====================" << std::endl;

		std::cout << "Queue size: " << sz << std::endl;
	}

	for (ATM& atm : atms)
		atm.thread.join();

	return 0;
}