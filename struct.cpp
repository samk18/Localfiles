#include <iostream>
using namespace std;
#include <string.h>
#include <cstring>
#define iop(u) ((void)(u))
struct Person
{
    int age;
    float master;
    int age1;
    char name[50];
    int salary;
};

struct Man
{
    
   int cast;
   int a;
   int b;
   int c;
   char name[7];
   float salary;
   int age1;

};

int main()
{   
    iop(2);
   (void)(7);
   
    Person p1;

char *op= (char *)malloc(8*(sizeof(float)+sizeof(int)));

void *lo=malloc(8*(sizeof(int)));

//memset(&lo,1,8*(sizeof(int)) );

//std::cout << *lo<< endl;

int *pok=(int*)lo;

// std::cout <<*pok<< endl;

// std::cout << "124569808374748"<< endl;
// std::cout << sizeof(op)<< endl;
// std::cout << sizeof(int)<< endl;

 char song[] = "We123";

  // print the length of the song string
  cout << strlen(song);
    p1.age = 15;
    p1.master = 20.908;
    p1.salary = 200;
strcpy( p1.name, "Air" );
char * str = "somethikllng";
char * str1 = "12345";

int* x=(int *)str1;
char * str2 =(char *)x;

 printf("  1234567845753    ");
std::cout<<*str2;

 printf("  1234567845753");
*str++ = *str1;
char abc[10] = "123";
if(*str == '1')
{
    printf("123456");
}

abc[0] = *str;
std::cout << abc[0]<< endl;
std::cout << "12457891something"<< endl;
//*str++ = '1';
   std::cout << str<< endl;
   std::cout << *str<< endl;
   std::cout << &p1.name<< endl;
   std::cout << &p1.age<< endl;
   std::cout << sizeof(float)<< endl;
std::cout << "12457891something1234"<< endl;

//std::cout << *str<< endl;
float c=20.908;
void *z=&c;
int* a=(int*)z;
 std::cout << (int)*a<< endl;



Man *m1 = (Man*)&p1;
cout <<"Age: nijam cast ayindi ra1 " << (m1->name) << endl;
cout <<"Age: nijam cast ayindi ra2 " << &(m1->age1) << endl;
cout <<"Age: nijam cast ayindi ra3 " << &(m1->salary) << endl;

cout <<"Age: nijam cast ayindi ra 4" << (m1->name) << endl;
cout <<"Age: nijam cast ayindi ra 5" << (m1->age1) << endl;
cout <<"Age: nijam cast ayindi ra 6 " << (m1->a) << endl;

Person *p2 = (Person *)(malloc(sizeof(Person)));
p2=&p1;
str=(char *)p2;
std::cout << p2<< endl;
std::cout << &p1<< endl;
std::cout << (void *)str<< endl;


Person* p3 = (Person*)str;

std::cout << (void *)p3<< endl;
std::cout << p3->age<< endl;
printf("%p qw", (void *)p3 );


char *re=NULL;
printf("pleasemakemeunderstand \n");
std::cout <<((char *)p3+60)<< endl;


// re = (char *)p2+sizeof(Person)+10;
// printf("%d",(void *)re );
// std::cout <<(void *)re<< endl;

// std::cout <<re<< endl;

 /*   cout << "Enter Full name: ";
    cin.get(p1.name, 50);
    cout << "Enter age: ";
    cin >> p1.age;
    cout << "Enter salary: ";
    cin >> p1.salary;     */

    cout << "\nDisplaying Information." << endl;
    cout << "Name: " << p1.name << endl;
    cout <<"Age: " << p1.age << endl;
    cout << "Salary: " << p1.salary;

    return 0;
}