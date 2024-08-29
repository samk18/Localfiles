#include <iostream>
using namespace std;

int doubt=234;
class A // just an example
{
public:
    int x=0;
public:
    A(){ std::cout<<x<<" ";
   
    x+=1; }
    int getX(int a) { x+=1; std::cout<<x; return x; }
};

class B: public A
{
public:
     A k;
    static B obj1;  // <- Problem happens here
public:
    B()
    { std::cout<<"hello  ";
     
   }
     static B& start()
    {
       //  std::cout<<"hello";
    return obj1;
    }
   using A::getX;
     int getX() {
        obj1.getX(3);
std::cout<<x<<" ";
        return 0; }
};
B B::obj1;

int main()
{
B a;
a.obj1.getX();
// std::cout<<a.obj1.x;

 //a.getX();
//      B::start().getX();
//   a.start().getX();
// std::cout<<a.obj1.x;


}

#include <iostream>
#include <memory>

class Car; // Forward declaration

class Person {
public:
    std::shared_ptr<Car> car;

    Person() {
        std::cout << "Person constructor" << std::endl;
    }

    ~Person() {
        std::cout << "Person destructor" << std::endl;
    }
};

class Car {
public:
    std::weak_ptr<Person> owner; // Use a weak pointer for the owner

    Car() {
        std::cout << "Car constructor" << std::endl;
    }

    ~Car() {
        std::cout << "Car destructor" << std::endl;
    }
};

int main() {
    // Create instances of Person and Car
    std::shared_ptr<Person> person = std::make_shared<Person>();
    std::shared_ptr<Car> car = std::make_shared<Car>();

    // Set the weak reference
    person->car = car;
    car->owner = person; // Use a weak_ptr here

    // Both shared pointers are still in scope
    std::cout << "Main function finished" << std::endl;

    return 0;
}



ST demo session
o   Legacy
o   Bbiff
o   Etsi
o   PCC
