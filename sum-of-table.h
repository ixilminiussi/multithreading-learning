#ifndef SUM_OF_TABLE
#define SUM_OF_TABLE

#include <cmath>
#include <mutex>
#include <thread>
#include <vector>

namespace sum_of_table
{

inline void sum(const std::vector<float> &table, float &r, size_t start = 0, size_t end = 1000)
{
    r = 0.0f;
    auto it_start = table.begin() + start;
    auto it_end = table.begin() + std::min(end, table.size());

    while (it_start != it_end)
    {
        r += *it_start;

        it_start++;
    }
}

inline void sumMutex(const std::vector<float> &table, float &r, std::mutex &m, size_t start = 0, size_t end = 1000)
{
    auto it_start = table.begin() + start;
    auto it_end = table.begin() + std::min(end, table.size());

    while (it_start != it_end)
    {
        {
            std::lock_guard<std::mutex> lock(m);
            r += *it_start;
        }

        it_start++;
    }
}

inline float sequencial(const std::vector<float> &table)
{
    float r{0};
    sum(table, r);
    return r;
}

inline float parallel(const std::vector<float> &table, size_t threadCount)
{
    if (threadCount == 0)
    {
        throw("threadCount cannot not be 0");
    }
    if (threadCount > table.size())
    {
        throw("threadCount should not be larger than the table size");
    }

    size_t split = std::floor(table.size() / threadCount);

    std::vector<std::thread> threads;
    std::vector<float> sums(threadCount);

    for (int i = 0; i < threadCount; i++)
    {
        size_t start = i * split;
        size_t end = (i == threadCount - 1) ? table.size() : (i + 1) * split;
        threads.push_back(std::thread(sum, std::ref(table), std::ref(sums[i]), start, end));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    float final_r = 0;
    sum(sums, final_r);

    return final_r;
}

inline float parallelMutex(const std::vector<float> &table, size_t threadCount)
{
    if (threadCount == 0)
    {
        throw("threadCount cannot not be 0");
    }
    if (threadCount > table.size())
    {
        throw("threadCount should not be larger than the table size");
    }

    size_t split = std::floor(table.size() / threadCount);

    std::vector<std::thread> threads;
    std::mutex m;
    float result = 0;

    for (int i = 0; i < threadCount; i++)
    {
        size_t start = i * split;
        size_t end = (i == threadCount - 1) ? table.size() : (i + 1) * split;
        threads.push_back(std::thread(sumMutex, std::ref(table), std::ref(result), std::ref(m), start, end));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    return result;
}

} // namespace sum_of_table

#endif
