#include <stdio.h>
#include "Pool.h"
#include "TaskQueue.h"
#include "DBDatabase.h"
#include "DBStatement.h"

using namespace POOL;

using namespace QUEUE;
class B
{
    public:
        B(){printf("B\n");}
        virtual int process(void* arg){return 0;};
        virtual ~B(){printf("B\n");}
};
class A : public B
{
    public:
        A(){}
        ~A(){}
        int process(void* arg)
        {
            char** argv = (char**)arg;
            for (int iLoop = 0; iLoop < 2; iLoop++)
            {
                printf("%s ", argv[iLoop]);
            }
            printf("\n");
            return 0;
        }
};

void* func(void* arg)
{
    pthread_rwlock_t* mmm = (pthread_rwlock_t*)arg;
    pthread_rwlock_wrlock(mmm);
    printf("1\n");
    return NULL;
}

int main(int argc, char** argv)
{
    /*pthread_t tid;
    pthread_rwlock_t m_mutexCount;
                    pthread_rwlock_wrlock(&m_mutexCount);
                    pthread_create(&tid, NULL, func, &m_mutexCount);
                    pthread_rwlock_wrlock(&m_mutexCount);*/
    CThreadPool threadpool(2, task_priority_queue);
    char conn[3][20] = {"system", "system", "tsdc"};
    char *value[3];
    value[0] = conn[0];
    value[1] = conn[1];
    value[2] = conn[2];
    CConnectPool<CDBDatabase> connnectpool(5);
    connnectpool.SetFunc(&CDBDatabase::Connect, &CDBDatabase::Disconnect, value);
    int count = 0;
    A p;
    CTaskQueue<CConnectPool<CDBDatabase> >* atask = new CTaskQueue<CConnectPool<CDBDatabase> >;
    char se[10] = "10000000";
    char *s[2];
    s[0] = se;
    s[1] = NULL;
    atask->SetFunc(Highest, &connnectpool, &CConnectPool<CDBDatabase>::ControlThread, s);
    task_priority_queue.push(atask);
    CDBDatabase *temp = connnectpool.GetConnection();
    CDBDatabase *temp2 = connnectpool.GetConnection();
    CDBStatement* dbstatement = new CDBStatement(temp);
    CDBStatement* dbs = new CDBStatement(temp2);
    dbstatement->Prepare("select city_code from bb_device_rent_info_t where user_id = 48303537");
    dbs->Prepare("select user_id from bb_device_rent_info_t where user_id = 48303537");
    dbstatement->Open();
    dbs->Open();
    if (dbstatement->Next() && dbs->Next())
    {
        printf("%s\n", dbstatement->Field(0).AsString());
        printf("%s\n", dbs->Field(0).AsString());
    }
    connnectpool.BackConnection(temp);
    connnectpool.BackConnection(temp2);
    while(1)
    {
    char time[2][10];
    char *ttt[3];
    sprintf(time[0], "time:");
    sprintf(time[1], "%d", count++);
    ttt[0] = time[0];
    ttt[1] = time[1];
    ttt[2] = NULL;
    CTaskQueue<A>* task = new CTaskQueue<A>;
    task->SetFunc(Low, &p, &A::process, (void*)ttt);
    task_priority_queue.push(task);
    usleep(1000000);
    }
    
    A a;
    CTaskQueue<A>* task = new CTaskQueue<A>;
    //usleep(100000);
    char aaa[2][10] = {"aaa", "bbb"};
    char *aa[3];
    char *bb[3];
    aa[0] = aaa[0];
    aa[1] = aaa[1];
    aa[2] = NULL;
    task->SetFunc(Low, &a, &A::process, (void*)aa);
    task_priority_queue.push(task);
    char bbb[2][10] = {"ccc", "bbb"};
    bb[0] = bbb[0];
    bb[1] = bbb[1];
    bb[2] = NULL;
    task = new CTaskQueue<A>;
    task->SetFunc(Minimum, &a, &A::process, (void*)bb);
    //task_priority_queue.push(task);
    
    //threadpool.ClearQueue();
    connnectpool.Clear();
    usleep(30000);
    threadpool.Clear();

    return 0;
}
