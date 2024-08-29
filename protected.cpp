using namespace std;
#include <iostream>
class Parent {  
  public:  
    Parent () //Constructor
    {
        cout << "\n Parent constructor called\n" << endl;
    }
  public:
    ~Parent() //Dtor
    {
        cout << "\n Parent destructor called\n" << endl;
    };
};


class Child : public Parent 
{
  private:
      static Child ch;
  public:
    int i=0;
    
    Child () //Ctor
    {
        cout << "\nChild constructor called\n" << endl;
    }
    static Child& hello()
    {
       
       cout << "\nChild constructefjwhgefuiwegfor called\n" << endl;
       return ch;
    }
    void hello1()
    {
         i=5;
         cout << i << endl;
       cout << "\nChild construdfctefjwhgefuiwegfor called111\n" << endl;

    }

    void hello2()
    {
         i=5;
         cout << this->i<< endl;
       cout << "\nChild construdfctefjwhgefuiwegfor called111\n" << endl;

    }
    protected:
    ~Child() //dtor
    {
        cout << "\nChild destructor called\n" << endl;
    }
};
Child Child::ch;
int main ()
{
    Parent * p2 = new Child;
    //Child c1;          
   Child::hello().hello1();
   delete p2;
    return 0;
}