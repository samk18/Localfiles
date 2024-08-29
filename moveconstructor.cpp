#include <iostream>
#include <string>

class AnotherClass {
public:
    AnotherClass(int value) : anotherValue(value) {
        std::cout << "AnotherClass constructor called. Value: " << anotherValue << std::endl;
    }

    // Move constructor
    AnotherClass(AnotherClass&& other) noexcept : anotherValue(other.anotherValue) {
        other.anotherValue = 0;  // Reset the source object to a valid state
        std::cout << "AnotherClass move constructor called. Moved Value: " << anotherValue << std::endl;
    }

    void displayValue() {
        std::cout << "AnotherClass value: " << anotherValue << std::endl;
    }

private:
    int anotherValue;
};

class MyClass {
public:
    // Default constructor
    MyClass(AnotherClass&& another) : intValue(0), stringValue("Default"), newMember(42), anotherObj(std::move(another)) {
        std::cout << "Default constructor called." << std::endl;
    }

    // Constructor with one parameter
    MyClass(int value, AnotherClass&& another) : intValue(value), stringValue("Default"), newMember(42), anotherObj(std::move(another)) {
        std::cout << "Parameterized constructor with int called. Value: " << intValue << std::endl;
    }

    // Constructor with two parameters
    MyClass(std::string text, double floatingValue, AnotherClass&& another) : intValue(0), stringValue(text), doubleValue(floatingValue), newMember(42), anotherObj(std::move(another)) {
        std::cout << "Parameterized constructor with string and double called. Text: " << stringValue << ", Double: " << doubleValue << std::endl;
    }

    // Member function to display values
    void displayValues() {
        std::cout << "Values: " << intValue << ", " << stringValue << ", " << doubleValue << ", " << newMember << std::endl;
        anotherObj.displayValue();
    }

private:
    // Member variables
    int intValue;
    std::string stringValue;
    double doubleValue;
    int newMember; // New member variable
    AnotherClass anotherObj; // Object of AnotherClass
};

int main() {
    // Creating an object of AnotherClass
    AnotherClass anotherObject(50);

    // Creating objects using different constructors, passing the AnotherClass object
    MyClass obj1(std::move(anotherObject));                      // Calls the default constructor
    MyClass obj2(42, std::move(anotherObject));                  // Calls the constructor with one int parameter
    MyClass obj3("Hello", 3.14, std::move(anotherObject));       // Calls the constructor with a string and a double parameter

    // Displaying values using a member function
    obj1.displayValues();
    obj2.displayValues();
    obj3.displayValues();

    return 0;
}
chatgpt
https://chat.openai.com/c/d1851aac-d1a7-45a5-a5ab-afc58bbbed90


https://chat.openai.com/c/f03347bf-0a9f-4d84-8759-62270b9837e8


