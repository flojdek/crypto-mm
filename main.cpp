#include <iostream>
#include <thread>

#include "src/spsc_queue/spsc_queue_v1.h"

using namespace std::literals::chrono_literals;

struct X {
  int x;
};

auto consumeFunction(utils::SpscQueueV1<X>& lfq) {
  std::this_thread::sleep_for(1s);

  while (lfq.size()) {
    const auto d = lfq.getNextToRead();

    std::cout << "read X{x=" << d->x << "}" << std::endl;

    lfq.updateReadIndex();
    std::this_thread::sleep_for(0.1s);
  }

  std::cout << "consumeFunction exiting." << std::endl;
}

int main()
{
  const int N = 10;
  utils::SpscQueueV1<X> qv1(N);

  std::thread t{[&qv1] { consumeFunction(qv1); }};

  for (auto i = 0; i < 2 * N; i++) {
    std::cout << "write " << i << std::endl;
    *(qv1.getNextToWrite()) = X{.x = i};
    qv1.updateWriteIdx();
  }

  while (true) {
    std::this_thread::sleep_for(1s);
  }

  return 0;
}
