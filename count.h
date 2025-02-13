#ifndef COUNT
#define COUNT

#include <condition_variable>
#include <iostream>
#include <thread>

namespace count
{

std::condition_variable cv;
bool isEvenTurn{true};
std::mutex mtx;

inline void countEven(int end, int &count)
{
    while (count < end)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return isEvenTurn; });

        std::cout << count << ", ";
        count++;
        isEvenTurn = false;

        cv.notify_one();
    }
}

inline void countUneven(int end, int &count)
{
    while (count < end)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !isEvenTurn; });

        std::cout << count << ", ";
        count++;
        isEvenTurn = true;

        cv.notify_one();
    }
}

inline void parallel()
{
    int count{0};
    std::thread t0(countEven, 1000, std::ref(count));
    std::thread t1(countUneven, 1000, std::ref(count));

    t0.join();
    t1.join();
}

} // namespace count

#endif
