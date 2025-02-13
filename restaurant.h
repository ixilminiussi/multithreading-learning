#ifndef RESTAURANT
#define RESTAURANT

#include <condition_variable>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace restaurant
{

enum Ingredient
{
    Potato,
    Garlic,
    Bread,
    Cucumber,
    Shrimp,
    Rice,
    Ham,
    Avocado,
    Spice,
    Tomato,
};

struct Meal
{
    std::shared_ptr<class Customer> customer;
    Ingredient ingredients[3];
};

class Actor
{
  public:
    void startThread();
    virtual void joinThread();

    virtual void update() = 0;

    std::thread *thread{nullptr};
};

class Customer : public Actor, public std::enable_shared_from_this<Customer>
{
  public:
    Customer();

    void update() override;

    void order();
    void serve(const Meal &meal);
    void eat();
    void exit();

    unsigned int ID;

  private:
    bool _isServed = false;
    Meal _meal;

    std::condition_variable _cv;

    static unsigned int customerID;
};

class Waiter : public Actor
{
  public:
    enum State
    {
        FREE,
        TO_KITCHEN,
        TO_CLIENT,
        CALLED,
    };

    Waiter();

    void update() override;
    void joinThread() override;

    void giveOrder(const Meal &meal);
    void handOver(const Meal &meal);

    void bringMeal(const Meal &meal);

    bool isAvailable() const
    {
        return state == State::FREE;
    }
    void makeBusy();

    unsigned int ID;
    static unsigned int availableCount;

  private:
    volatile State state{FREE};
    Meal _heldMeal{};

    std::condition_variable _cv;

    static unsigned int waiterID;
};

class Cook : public Actor
{
  public:
    Cook();

    void update() override;

    void prepare(const Meal &meal) const;

    unsigned int ID;

  private:
    static unsigned int cookID;
};

class Chief : public Actor
{
  public:
    void update() override;

    void mix(const Meal &meal);

    std::queue<Meal> chiefQueue{};
};

class Restaurant
{
  public:
    static Restaurant *getInstance();

    void initialize();
    void close();

    std::shared_ptr<Waiter> callForWaiter();

    void addCustomer(std::shared_ptr<Customer>);
    void removeCustomer(std::shared_ptr<Customer>);
    bool hasCustomers()
    {
        return _customers.size() > 0;
    }

    void addWaiter(std::shared_ptr<Waiter>);
    void removeWaiter(std::shared_ptr<Waiter>);

    void addCook(std::shared_ptr<Cook>);
    void removeCook(std::shared_ptr<Cook>);

    void setChief(std::shared_ptr<Chief>);
    std::shared_ptr<Chief> getChief()
    {
        return _chief;
    }

    void logThreadSafe(const std::string &) const;

    std::queue<Meal> kitchenQueue{};

    std::condition_variable kitchenCv;
    std::mutex kitchenMtx;

    std::condition_variable waiterCv;

    volatile bool closeRestaurant{false};

  private:
    Restaurant();
    ~Restaurant();

    static Restaurant *instance;

    std::vector<std::shared_ptr<Customer>> _customers{};
    std::vector<std::shared_ptr<Waiter>> _waiters{};
    std::vector<std::shared_ptr<Cook>> _cooks{};
    std::shared_ptr<Chief> _chief{nullptr};
};

inline void log(const Restaurant *restaurant, const std::ostringstream &oss)
{
    std::string final = "\033[31m Restaurant : \033[0m" + oss.str();

    restaurant->logThreadSafe(final);
}

inline void log(const Customer *customer, const std::ostringstream &oss)
{
    Restaurant *restaurant = Restaurant::getInstance();
    std::string final = "\033[32m Customer " + std::to_string(customer->ID) + " : \033[0m" + oss.str();

    restaurant->logThreadSafe(final);
}

inline void log(const Cook *cook, const std::ostringstream &oss)
{
    std::string final = "\033[33m Cook " + std::to_string(cook->ID) + " : \033[0m" + oss.str();
    Restaurant *restaurant = Restaurant::getInstance();

    restaurant->logThreadSafe(final);
}

inline void log(const Chief *chief, const std::ostringstream &oss)
{
    std::string final = "\033[34m Chief : \033[0m" + oss.str();
    Restaurant *restaurant = Restaurant::getInstance();

    restaurant->logThreadSafe(final);
}

inline void log(const Waiter *waiter, const std::ostringstream &oss)
{
    std::string final = "\033[35m Waiter " + std::to_string(waiter->ID) + " : \033[0m" + oss.str();
    Restaurant *restaurant = Restaurant::getInstance();

    restaurant->logThreadSafe(final);
}
} // namespace restaurant

#endif
