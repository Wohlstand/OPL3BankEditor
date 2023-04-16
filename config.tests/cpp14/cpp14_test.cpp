//
// c++ 11
//
// http://bfilipek.hubpages.com/hub/What-you-should-know-about-C11
// http://bfilipek.hubpages.com/hub/What-you-should-know-about-C11-Part-II
// http://bfilipek.hubpages.com/hub/What-you-should-know-about-C11-Part-III
//
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

//
// variadic templates
//
template<typename T>
T adder(T v)
{
    return v;
}

template<typename T, typename... Args>
T adder(T first, Args... args)
{
    return first + adder(args...);
}

//
// universal references
//
template<typename T>
void func(const T& rvalue)
{
    std::cout << rvalue << std::endl;
}
template<typename T>
void func(T& lvalue)
{
    std::cout << lvalue << std::endl;
}

template <class T>
void wrapper(T&& t)
{
    func(std::forward<T>(t));
}

//
// nullptr
//
void test(int* p)
{
    std::cout << "pointer " << p << std::endl;
}

void test(int x)
{
    std::cout << "integer " << x << std::endl;

    wrapper(x);
}

//
// enum
//
enum class CarType
{
    None,
    Sport,
    Coupe,
    Van
};

//
// classes
//
class ICar
{
    virtual void Drive(int distance) { }
    virtual void Stop() { }
};

class Car final : public ICar
{
private:
    std::string mName = "none"; //{ "none" }; //not fully implemented
    int         mAge{ 0 };
    CarType     mType{ CarType::None };


    Car(const Car&) = delete;
    Car& operator=(const Car&) = delete;
public:
    Car(const std::string& name)
        : mName(name)//: mName{ name } //not fully implemented
    {
    }

    Car(const std::string& name, int age)
        : mName(name)//: mName{ name } //not fully implemented
        , mAge{ age }
    {
    }

    Car(const std::string& name, int age, CarType ct)
        : mName(name) //: mName{ name } //not fully implemented
        , mAge{ age }
        , mType{ ct }
    {
    }

    virtual ~Car() { }

    //implement ICar
    /*virtual*/ void Drive(int distance) override { }
    /*virtual*/ void Stop() override { }
};



int main(int argc, char* argv[])
{
    //variadic templates
    long sum = adder(1, 2, 3);

    std::string s1 = "x", s2 = "aa", s3 = "bb", s4 = "yy";
    std::string ssum = adder(s1, s2, s3, s4);

    //universal references
    int x = 9;
    wrapper(x);
    wrapper(7);

    //uniform init + initializer list, auto, for, lambda
    std::vector<double> vecMarks{ 0.0, 1.0, 2.0, 3.0 };

    std::sort(begin(vecMarks), end(vecMarks),
              [](const double & a, const double & b)
    {
        return b < a;
    });

    for(const auto& elem : vecMarks)
        std::cout << elem << std::endl;

    //nullptr
    test(7);
    test(nullptr);

    //
    // lambdas c++14
    //
    // http://cpptruths.blogspot.ro/2014/03/fun-with-lambdas-c14-style-part-1.html
    // http://cpptruths.blogspot.ro/2014/05/fun-with-lambdas-c14-style-part-2.html
    // http://cpptruths.blogspot.ro/2014/08/fun-with-lambdas-c14-style-part-3.html
    //
    auto list = [](auto ...xs)
    {
        return [ = ](auto access) { return access(xs...); };
    };

    auto head = [](auto xs)
    {
        return xs([](auto first, auto ...rest) { return first; });
    };

    auto tail = [list](auto xs)
    {
        return xs([list](auto first, auto ...rest) { return list(rest...); });
    };

    auto length = [](auto xs)
    {
        return xs([](auto ...z) { return sizeof...(z); });
    };


    //
    //threads
    //
    const int NUM_THREADS = 20;

    std::thread myThreads[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; ++i)
    {
        myThreads[i] = std::thread([i]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Hello, " << " thread: " << i << std::endl;
            std::this_thread::yield();
        });
    }

    for(auto& t : myThreads)
        t.join();

    std::cout << "All threads are done..." << std::endl;

    return 0;
}