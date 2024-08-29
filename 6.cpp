class Task
{
 public:
     int *mId;
    Task(int id ) 
    {
        mId=new int;
        *mId=id;
        std::cout<<"Task::Constructor"<<std::endl;
    }
    ~Task()
    {
        delete mId;
        std::cout<<"Task::Destructor"<<std::endl;
    }
};

class A
{
 public:
   ~A()
   {

   };
};

class Task : public A
{
 public:
    std::unique_ptr<int> mId;
    int i=4;
    
    Task(int id ) 
    {
      
        mId=std::make_unique<int>();
        *mId=id;
        std::cout<<"Task::Constructor"<<std::endl;
    }
   
   
};

int main()
{
 std::unique_ptr<A> taskPtr=std::make_unique<Task>(23);
}