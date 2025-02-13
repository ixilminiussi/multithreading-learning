#ifndef DOUBLE_HELLO
#define DOUBLE_HELLO

#include <iostream>
#include <thread>

namespace say_hello
{

inline void sayHello()
{
    std::cout << "Hello World" << std::endl;
}

inline void parallel()
{
    std::thread t0(sayHello);
    std::thread t1(sayHello);
    std::thread t2(sayHello);

    t0.join();
    t1.join();
    t2.join();
}

} // namespace say_hello

#endif
