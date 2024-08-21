#include<iostream>
#include<thread>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<future>

using namespace std;

int func(int a) {

	this_thread::sleep_for(chrono::seconds(2));
	cout << "this is time taking function" << endl;

	return a * a;
}

class Threadpool {
private:
	int m_threads;
	vector<thread> mem;
	queue<function<void()>> tasks;
	mutex mtx;
	condition_variable cvb;
	bool stop;
public:

	explicit Threadpool(int numThreads) : m_threads(numThreads) , stop(false){

		for (int i = 0; i < m_threads; i++) {
			mem.emplace_back([this] {
			function<void()> task;
				while (1) {
					unique_lock<mutex> lock(mtx);
					cvb.wait(lock, [this] {
						return !tasks.empty() || stop;
						});

					if (stop) return;

					task = move(tasks.front());
					tasks.pop();
					cout << "Size of queue : " << tasks.size() << endl;

					lock.unlock();
					task();
				}
				});
		}
	}

	~Threadpool() {
		unique_lock<mutex> lock(mtx);
		stop = true;

		lock.unlock();
		cvb.notify_all();

		for (auto& th : mem) {
			th.join();
		}
	}


	template<class F , class... Args>
	auto ExecuteTask(F&& f , Args&&... args) -> future<decltype(f(args...))> {

		using return_type = decltype(f(args...));

		auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f),forward<Args>(args)...));

		future<return_type> res = task->get_future();
		
		unique_lock<mutex> lock(mtx);

		tasks.emplace([task]() -> void{
			(*task)();
			});

		lock.unlock();

		cvb.notify_one();

		return res;
	}

};

int main() {

	Threadpool pool(16);

	int num = 2;


	while (num<21) {
		future<int> res = pool.ExecuteTask(func, num);
		cout << "result : " << res.get() << endl;

		num += 2;
	}

	//cout << "threadpool program started" << endl;
	return 0;

}