#include "count.h"
#include "double-hello.h"
#include "matrix.h"
#include "restaurant.h"
#include "sum-of-table.h"

#include <sstream>
#include <string>

template <typename T> std::string str(T begin, T end)
{
    std::stringstream ss;
    bool first = true;
    for (; begin != end; begin++)
    {
        if (!first)
            ss << ", ";
        ss << *begin;
        first = false;
    }
    return ss.str();
}

int main()
{
    say_hello::parallel();
    std::cout << std::endl << std::endl;

    matrix::Matrix a{1, 4};
    a(0, 0) = 1;
    a(0, 1) = 2;
    a(0, 2) = 3;
    a(0, 3) = 4;
    std::cout << "Matrix A: " << std::endl << a << std::endl;

    matrix::Matrix b{4, 1};
    b(0, 0) = 4;
    b(1, 0) = 5;
    b(2, 0) = 6;
    b(3, 0) = 7;
    std::cout << "Matrix B: " << std::endl << b << std::endl;

    std::cout << "A x B (sequencial): " << std::endl << matrix::sequencial(a, b) << std::endl;
    std::cout << "A x B (parallel): " << std::endl << matrix::parallel(a, b) << std::endl;

    std::cout << std::endl;

    std::vector<float> table = {4, 12, 35, -44, -125, 675, 70, 8, 19, 45, 531, -678, 1, -1, 0, 1005, 32, -53};

    std::cout << "Table : " << str(table.begin(), table.end()) << std::endl;
    std::cout << "Sum (sequencial): " << sum_of_table::sequencial(table) << std::endl;
    std::cout << "Sum (parallel): " << sum_of_table::parallel(table, 9) << std::endl;
    std::cout << "Sum (parallel mutex): " << sum_of_table::parallelMutex(table, 5) << std::endl;

    std::cout << std::endl << std::endl;

    std::cout << "Count (parallel): ";
    count::parallel();
    std::cout << std::endl;

    std::cout << std::endl;

    restaurant::Restaurant *restaurant = restaurant::Restaurant::getInstance();
    restaurant->initialize();
    restaurant->close();

    return 0;
}
