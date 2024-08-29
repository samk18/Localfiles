#include <iostream>
#include <stdexcept>
#include <vector>

#define MAKE_FAILURE 0

static unsigned count;
const unsigned ExceptionThreshold = 3;
struct B {
    B() { if(ExceptionThreshold <= count++) throw std::runtime_error("Sorry"); }
    ~B() { std::cout << "Destruct\n"; }
};

struct A
{
    std::vector<B*> v;
    A()
    {
        for(unsigned i = 0; i < ExceptionThreshold + MAKE_FAILURE; ++i)
            v.push_back(new B);
    }

    ~A()
    {
        for(unsigned i = 0; i < v.size(); ++i) {
            delete v[i];
        }
    }
};

int main()  
{
    A a;

    
    return 0;
}