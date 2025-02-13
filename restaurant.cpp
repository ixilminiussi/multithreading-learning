#include "restaurant.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>

namespace restaurant
{

// Meal Begin
const char *ingredientToString(Ingredient ingredient)
{
    switch (ingredient)
    {
    case Potato:
        return "Potato";
        break;
    case Garlic:
        return "Garlic";
        break;
    case Bread:
        return "Bread";
        break;
    case Cucumber:
        return "Cucumber";
        break;
    case Shrimp:
        return "Shrimp";
        break;
    case Rice:
        return "Rice";
        break;
    case Ham:
        return "Ham";
        break;
    case Avocado:
        return "Avocado";
        break;
    case Spice:
        return "Spice";
        break;
    default:
        return "Tomato";
        break;
    }
}

std::ostream &operator<<(std::ostream &os, const Meal &meal)
{
    os << "[" << ingredientToString(meal.ingredients[0]) << ", " << ingredientToString(meal.ingredients[1]) << ", "
       << ingredientToString(meal.ingredients[2]) << "]";
    return os;
}
// Meal End

// Actor Begin
void Actor::startThread()
{
    auto update_func = [&]() { update(); };

    thread = new std::thread(update_func);
}

void Actor::joinThread()
{
    thread->join();
}
// Actor End

// Customer Begin
unsigned int Customer::customerID{0};

Customer::Customer()
{
    ID = customerID++;
}

void Customer::update()
{
    order();

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    _cv.wait(lock, [&] { return _isServed; });

    eat();
    exit();
}

void Customer::order()
{
    static std::vector<Ingredient> options = {Potato, Garlic, Bread,   Cucumber, Shrimp,
                                              Rice,   Ham,    Avocado, Spice,    Tomato};
    static std::random_device rd;
    static std::mt19937 g(rd());

    std::shuffle(options.begin(), options.end(), g);
    Meal meal{shared_from_this(), options[0], options[1], options[2]};

    Restaurant *restaurant = Restaurant::getInstance();

    {
        std::ostringstream oss;
        oss << "waiting to order";
        log(this, oss);
    }

    std::shared_ptr<Waiter> waiter = restaurant->callForWaiter();

    {
        std::ostringstream oss;
        oss << "ordered " << meal << " from waiter " << waiter->ID;
        log(this, oss);
    }

    waiter->giveOrder(meal);
}

void Customer::serve(const Meal &meal)
{
    std::ostringstream oss;
    oss << "served " << meal;
    log(this, oss);

    _meal = meal;
    _isServed = true;
    _cv.notify_one();
}

void Customer::eat()
{
    std::ostringstream oss;
    oss << "eating meal " << _meal;
    log(this, oss);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void Customer::exit()
{
    std::ostringstream oss;
    oss << "leaves restaurant happily";
    log(this, oss);

    Restaurant *restaurant = Restaurant::getInstance();

    restaurant->removeCustomer(shared_from_this());
}
// Customer End

// Waiter Begin
unsigned int Waiter::availableCount{0};
unsigned int Waiter::waiterID{0};

Waiter::Waiter()
{
    ID = waiterID++;
    state = State::FREE;
    availableCount++;
}

void Waiter::update()
{
    Restaurant *restaurant = Restaurant::getInstance();

    while (!restaurant->closeRestaurant)
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lock{mtx};
        _cv.wait(lock, [&] {
            return restaurant->closeRestaurant || state == State::TO_CLIENT || state == State::TO_KITCHEN;
        });

        if (restaurant->closeRestaurant)
            return;

        switch (state)
        {
        case TO_KITCHEN: {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::ostringstream oss;
            oss << "added meal " << _heldMeal << " to kitchen queue";
            log(this, oss);

            std::lock_guard<std::mutex> lock(restaurant->kitchenMtx);
            restaurant->kitchenQueue.push(_heldMeal);
            restaurant->kitchenCv.notify_one();
        }
        break;
        case TO_CLIENT: {
            std::ostringstream oss;
            oss << "bringing meal to Customer..." << _heldMeal.customer->ID;
            log(this, oss);

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            _heldMeal.customer->serve(_heldMeal);
        }
        break;
        }

        state = FREE;
        availableCount++;

        restaurant->waiterCv.notify_one();
    }
}

void Waiter::makeBusy()
{
    state = CALLED;
    availableCount--;

    if (availableCount > 0)
        Restaurant::getInstance()->waiterCv.notify_one();
}

void Waiter::joinThread()
{
    _cv.notify_one();

    thread->join();
}

void Waiter::giveOrder(const Meal &meal)
{
    _heldMeal = meal;
    state = TO_KITCHEN;

    std::ostringstream oss;
    oss << "received meal order from customer " << meal.customer->ID << " : " << meal;
    log(this, oss);

    _cv.notify_one();
}

void Waiter::handOver(const Meal &meal)
{
    _heldMeal = meal;
    state = TO_CLIENT;

    std::ostringstream oss;
    oss << "received prepared meal from chef to customer" << meal.customer->ID << " : " << meal;
    log(this, oss);

    _cv.notify_one();
}
// Waiter End

// Cook Begin
unsigned int Cook::cookID{0};

Cook::Cook()
{
    ID = cookID++;
}

std::mutex chiefQueueMtx;

void Cook::update()
{
    Restaurant *restaurant = Restaurant::getInstance();

    while (!restaurant->closeRestaurant)
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lock{mtx};
        restaurant->kitchenCv.wait(lock,
                                   [&] { return restaurant->closeRestaurant || restaurant->kitchenQueue.size() > 0; });

        if (restaurant->closeRestaurant)
            return;

        Meal meal;
        {
            std::lock_guard<std::mutex> queueLock(restaurant->kitchenMtx);
            meal = restaurant->kitchenQueue.front();
            restaurant->kitchenQueue.pop();
        }

        if (restaurant->kitchenQueue.size() > 0)
            restaurant->kitchenCv.notify_one();

        prepare(meal);

        std::lock_guard<std::mutex> chiefQueueLock(chiefQueueMtx);
        restaurant->getChief()->chiefQueue.push(meal);
    }
}

void Cook::prepare(const Meal &meal) const
{
    {
        std::ostringstream oss;
        oss << "preparing meal..." << meal;
        log(this, oss);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    {
        std::ostringstream oss;
        oss << "meal " << meal << " prepared!";
        log(this, oss);
    }
}
// Cook End

// Chief Begin
void Chief::update()
{
    Restaurant *restaurant = Restaurant::getInstance();

    while (!restaurant->closeRestaurant)
    {
        while (chiefQueue.size() == 0)
        {
            if (restaurant->closeRestaurant)
                return;
        };

        Meal meal;

        {
            std::lock_guard<std::mutex> chiefQueueLock(chiefQueueMtx);
            meal = chiefQueue.front();
            chiefQueue.pop();
        }

        mix(meal);

        std::shared_ptr<Waiter> waiter = restaurant->callForWaiter(); // FIX: nullptr return at times

        std::ostringstream oss;
        oss << "handed over meal " << meal << " to Waiter " << waiter->ID;
        log(this, oss);

        waiter->handOver(meal);
    }
}

void Chief::mix(const Meal &meal)
{
    {
        std::ostringstream oss;
        oss << meal << " mixing...";
        log(this, oss);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::ostringstream oss;
        oss << meal << " mixed!";
        log(this, oss);
    }
}
// Chief End

// Restaurant Begin
Restaurant *Restaurant::instance{nullptr};

Restaurant::Restaurant()
{
}

void Restaurant::initialize()
{
    for (int i = 0; i < 10; i++)
    {
        addCustomer(std::make_shared<Customer>());
    }
    for (int i = 0; i < 4; i++)
    {
        addCook(std::make_shared<Cook>());
    }
    for (int i = 0; i < 3; i++)
    {
        addWaiter(std::make_shared<Waiter>());
    }
    setChief(std::make_shared<Chief>());

    for (std::shared_ptr<Customer> customer : _customers)
    {
        customer->startThread();
    }

    for (std::shared_ptr<Cook> cook : _cooks)
    {
        cook->startThread();
    }

    for (std::shared_ptr<Waiter> waiter : _waiters)
    {
        waiter->startThread();
    }

    _chief->startThread();
}

void Restaurant::close()
{
    while (kitchenQueue.size() > 0 || _chief->chiefQueue.size() > 0 || Waiter::availableCount < _waiters.size() ||
           hasCustomers())
    {
    }

    {
        std::ostringstream oss;
        oss << "closing...";
        log(this, oss);
    }

    closeRestaurant = true;

    kitchenCv.notify_all();
    waiterCv.notify_all();

    for (auto customer : _customers)
    {
        customer->joinThread();
    }
    _customers.clear();

    {
        std::ostringstream oss;
        oss << "customers are gone";
        log(this, oss);
    }

    for (auto cook : _cooks)
    {
        cook->joinThread();
    }
    _cooks.clear();

    {
        std::ostringstream oss;
        oss << "cooks have left";
        log(this, oss);
    }

    for (auto waiter : _waiters)
    {
        waiter->joinThread();
    }
    _waiters.clear();

    {
        std::ostringstream oss;
        oss << "waiters are gone";
        log(this, oss);
    }

    _chief->joinThread();
}

Restaurant::~Restaurant()
{
}

std::shared_ptr<Waiter> Restaurant::callForWaiter()
{
    static std::mutex mtx;

    std::unique_lock<std::mutex> lock(mtx);
    waiterCv.wait(lock, [&] { return (Waiter::availableCount > 0); });

    for (std::shared_ptr<Waiter> waiter : _waiters)
    {
        if (waiter->isAvailable())
        {
            waiter->makeBusy();
            return waiter;
        }
    }

    return nullptr;
}

void Restaurant::addCustomer(std::shared_ptr<Customer> customer)
{
    _customers.push_back(customer);

    std::ostringstream oss;
    oss << std::to_string(_customers.size()) << " customers now.";
    log(this, oss);
}

void Restaurant::removeCustomer(std::shared_ptr<Customer> customer)
{
    auto it = std::find(_customers.begin(), _customers.end(), customer);

    if (it != _customers.end())
    {
        _customers.erase(it);
    }

    std::ostringstream oss;
    oss << std::to_string(_customers.size()) << " customers now.";
    log(this, oss);
}

void Restaurant::addWaiter(std::shared_ptr<Waiter> waiter)
{
    _waiters.push_back(waiter);
}

void Restaurant::removeWaiter(std::shared_ptr<Waiter> waiter)
{
    auto it = std::find(_waiters.begin(), _waiters.end(), waiter);

    if (it != _waiters.end())
    {
        _waiters.erase(it);
    }
}

void Restaurant::addCook(std::shared_ptr<Cook> cook)
{
    _cooks.push_back(cook);
}

void Restaurant::removeCook(std::shared_ptr<Cook> cook)
{
    auto it = std::find(_cooks.begin(), _cooks.end(), cook);

    if (it != _cooks.end())
    {
        _cooks.erase(it);
    }
}

void Restaurant::setChief(std::shared_ptr<Chief> chief)
{
    _chief = chief;
}

Restaurant *Restaurant::getInstance()
{
    static Restaurant instance;
    return &instance;
}

std::string getCurrentTime()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "[%H:%M:%S]");
    return oss.str();
}

void Restaurant::logThreadSafe(const std::string &str) const
{
    static std::mutex mtx;

    std::lock_guard<std::mutex> lock(mtx);

    std::cout << getCurrentTime() << str << std::endl;
}
// Restaurant End

} // namespace restaurant
