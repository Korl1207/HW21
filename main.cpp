#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

std::mutex coutMutex;

class TaskQueue {
private:
  std::queue<int> tasks;
  std::mutex mtx;
  std::condition_variable cv;
  bool stopped = false;

public:
  void push(int value) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      tasks.push(value);
    }
    cv.notify_one();
  }

  bool waitAndPop(int &value) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() { return !tasks.empty() || stopped; });
    if (stopped && tasks.empty()) {
      return false;
    }
    value = tasks.front();
    tasks.pop();

    return true;
  }
  void stop() {
    {
      std::lock_guard<std::mutex> lock(mtx);
      stopped = true;
    }
    cv.notify_all();
  }
};

void worker(TaskQueue &queue, int id) {
  while (true) {
    int task = 0;
    if (!queue.waitAndPop(task)) {
      std::lock_guard<std::mutex> lock(coutMutex);
      std::cout << "[Worker-" << id << "] finished work" << std::endl;
      break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
      std::lock_guard<std::mutex> lock(coutMutex);
      std::cout << "[Worker-" << id << "] processed task " << task << std::endl;
    }
  }
}

int main() {
  TaskQueue queue;
  const int N = 4;
  std::vector<std::thread> workers;

  for (int i = 0; i < N; i++)
    workers.emplace_back(worker, std::ref(queue), i + 1);

  for (int i = 1; i <= 20; i++)
    queue.push(i);
  queue.stop();

  for (auto &thread : workers)
    thread.join();

  std::cout << std::endl << "All tasks were finished" << std::endl;
  return 0;
}
