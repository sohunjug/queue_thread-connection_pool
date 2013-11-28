/*******************************************************************************
 * Author : RKhuwq
 * Email : sohunjug@hotmail.com
 * Last modified : 2013-11-18 16:55
 * Filename : Pool.h
 * Description : 
 * *****************************************************************************/

#ifndef __POOL_H__
#define __POOL_H__

#include <pthread.h>
#include <map>
#include <stdio.h>
#include "TaskQueue.h"
#include <signal.h>
//#include <time.h>

namespace POOL
{
    enum PoolStatue
    {
        Wait = 1,
        Kill
    };
    class CPoolBase
    {
        protected:
            int m_init;
            int m_iNum;
            int m_iCount;
            pthread_attr_t m_attr;
            virtual void  Create() = 0;
            virtual int   ControlThread(void* argv) = 0;
        public:
            PoolStatue statues;
            pthread_mutex_t m_mutex;
            pthread_mutexattr_t  m_mutexAttr;
            CPoolBase(int num, PoolStatue statue) : m_init(num), m_iNum(5), m_iCount(0), statues(statue)
        {
            pthread_attr_init(&m_attr);
            pthread_attr_setdetachstate(&m_attr,PTHREAD_CREATE_JOINABLE);
            pthread_mutexattr_init(&m_mutexAttr);
            pthread_mutex_init(&m_mutex, &m_mutexAttr);
        }
            virtual ~CPoolBase()
            {
                pthread_mutexattr_destroy(&m_mutexAttr);
            };
    };

    class CThreadPool : public CPoolBase
    {
        struct pthread_statue
        {
            pthread_t pthread_id;
            int statue;
            pthread_mutex_t m_threadMutex;
            pthread_cond_t  m_threadCond;
        };
        public:
        std::map<pthread_t, pthread_statue*> m_mthread;
        std::map<pthread_t, pthread_statue*>::iterator m_iter;
        QUEUE::queue_priority& queue;

        private:
        bool is_clear;
        pthread_mutex_t m_mutexCount;
        QUEUE::CTaskQueue<CThreadPool> *owntask;
        void Create()
        {
            pthread_statue *new_pthread = new pthread_statue();
            pthread_mutex_lock(&m_mutex);
            if(pthread_create(&new_pthread->pthread_id, &m_attr, ThreadMain, this)!=0)
            {
                return ;
            }
            m_mthread.insert(std::pair<pthread_t, pthread_statue*> (new_pthread->pthread_id, new_pthread));
            pthread_mutex_unlock(&m_mutex);
            pthread_mutex_lock(&m_mutexCount);
            m_iCount++;
            pthread_mutex_unlock(&m_mutexCount);
            //ThreadMain((void*)this);
        }
        int   ControlThread(void* argv)
        {
            int in_count_add = 0;
            int in_count_del = 0;
            int task_count;
            bool change;
            CThreadPool* pclass = (CThreadPool*)argv;
            pthread_statue *statue_self;
            std::map<pthread_t, pthread_statue*>::iterator iter;
            pthread_mutex_lock(&pclass->m_mutex);
            statue_self = pclass->m_mthread.find(pthread_self())->second;
            pthread_mutex_unlock(&pclass->m_mutex);
            statue_self->statue = 1;
            while(1)
            {
                while (m_iCount < m_init && m_iCount < m_iNum && ((statues == Wait && !queue.empty()) || !is_clear))
                {
                    Create();
                }
                task_count = queue.size();
                if (task_count)
                {
                    pthread_mutex_lock(&m_mutex);
                    iter = m_mthread.begin();
                    while (iter != m_mthread.end() && ((statues == Wait && !queue.empty()) || !is_clear))
                    {
                        if (iter->second->statue == 2 && iter->second->pthread_id != pthread_self())
                        {
                            pthread_cond_signal (&iter->second->m_threadCond);
                            break;
                        }
                        iter++;
                    }
                    if (iter == m_mthread.end())
                        m_iNum ++;
                    pthread_mutex_unlock(&m_mutex);
                    if (task_count > m_iCount)
                    {
                        in_count_add++;
                        in_count_del = 0;
                    }
                    else if (task_count < m_iCount)
                    {
                        //in_count_del++;
                        in_count_add = 0;
                    }
                    if (in_count_add > 10)
                    {
                        in_count_add = 0;
                        m_iNum++;
                        change = true;
                    }
                    if (in_count_del > 10 && m_iNum > m_init)
                    {
                        m_iNum--;
                        change = true;
                    }
                }
                /*if (change && m_iNum > m_init && in_count_del > 10)
                  {
                  pthread_mutex_lock(&m_mutex);
                  iter = m_mthread.begin();
                  while(iter != m_mthread.end() && !is_clear)
                  if (iter->first != pthread_self())
                  {
                  iter->second->statue = 0;
                  pthread_cond_signal (&iter->second->m_threadCond);
                  in_count_del = 0;
                  break;
                  }
                  else
                  {
                  iter++;
                  }
                  pthread_mutex_unlock(&m_mutex);
                  }*/
                if (task_count == 0)
                    usleep(100);

                if ((statue_self->statue == 0 || is_clear) && !(statues == Wait && !queue.empty()))
                {
                    pthread_mutex_lock(&m_mutexCount);
                    pclass->m_iCount--;
                    pthread_mutex_unlock(&m_mutexCount);
                    break;
                }
            }
            return 0;
        } 
        public:
        static void* ThreadMain(void *arg)
        {
            //pthread_detach(pthread_self());
            pthread_statue* statue_self;
            CThreadPool* pclass = (CThreadPool*)arg;
            QUEUE::CTaskBase* task = NULL;
            pthread_mutex_lock(&pclass->m_mutex);
            statue_self = pclass->m_mthread.find(pthread_self())->second;
            pthread_mutex_unlock(&pclass->m_mutex);
            pthread_mutex_init (&statue_self->m_threadMutex, &pclass->m_mutexAttr);
            pthread_cond_init (&statue_self->m_threadCond, NULL);
            statue_self->statue = 1;
            //timespec time;
            statue_self->pthread_id = pthread_self();
            while(1)
            { 
                if (pclass->statues != Wait || pclass->queue.empty())
                {
                    if (statue_self->statue == 0)
                    {
                        pthread_mutex_lock(&pclass->m_mutexCount);
                        pclass->m_iCount--;
                        pthread_mutex_unlock(&pclass->m_mutexCount);
                        break;
                    }
                }
                task = NULL;

                pthread_mutex_lock(&pclass->m_mutex);
                if(pclass->queue.empty())
                {
                    if (pthread_mutex_trylock(&statue_self->m_threadMutex) != 0)
                    {
                        pthread_mutex_unlock(&pclass->m_mutex);
                        continue;
                    }
                    pthread_mutex_unlock(&pclass->m_mutex);
                    //clock_gettime(0, &time);
                    //time.tv_sec += 1;
                    statue_self->statue = 2;
                    pthread_cond_wait(&statue_self->m_threadCond, &statue_self->m_threadMutex);
                    pthread_mutex_unlock(&statue_self->m_threadMutex);
                    continue;
                }

                task=pclass->queue.top();
                pclass->queue.pop();
                pthread_mutex_unlock(&pclass->m_mutex);

                if (task)
                {
                    statue_self->statue = 1;
                    if (task->Start() != 0)
                    {
                    }
                    if (!task)
                        delete task;
                }
            }
            //iter->second.statue = 3;
            return NULL;
        }
        void Stop(pthread_t pthread_id)
        {
            pthread_statue *temp;
            pthread_mutex_lock(&m_mutex);
            temp = m_mthread.find(pthread_id)->second;
            pthread_mutex_unlock(&m_mutex);
            if (!temp)
                return;
            while(pthread_kill(temp->pthread_id, 0) == 0)
            {
                pthread_mutex_lock(&temp->m_threadMutex);
                temp->statue = 0;
                pthread_mutex_unlock(&temp->m_threadMutex);
                pthread_cond_signal(&temp->m_threadCond);
                usleep(10000);
            }
            //pthread_join(temp->pthread_id, NULL);
            //if (m_iter->second.statue == 3)
            pthread_mutex_lock(&m_mutex);
            temp = m_mthread.find(pthread_id)->second;
            m_mthread.erase(pthread_id);
            pthread_mutex_unlock(&m_mutex);
            delete temp;
        }
        void ClearQueue() 
        { 
            while (!queue.empty())
            {
                delete queue.top();
                queue.pop();
            }
        }
        void Clear()
        {
            is_clear = true;
            pthread_mutex_lock(&m_mutex); 
            std::map<pthread_t, pthread_statue*>::iterator iter = m_mthread.begin();
            for (; iter != m_mthread.end(); iter = m_mthread.begin())
            {
                pthread_mutex_unlock(&m_mutex); 
                Stop(iter->first);
                pthread_mutex_lock(&m_mutex); 
            }
            pthread_mutex_unlock(&m_mutex); 
        }

        public:
        CThreadPool(int num, QUEUE::queue_priority& queue, PoolStatue Pool = Wait) : CPoolBase(num, Pool), queue(queue), is_clear(false) 
        { 
            pthread_mutex_init (&m_mutexCount, NULL);
            owntask = new QUEUE::CTaskQueue<CThreadPool>;
            owntask->SetFunc(QUEUE::Urgent, this, &CThreadPool::ControlThread, this);
            queue.push(owntask);
            Create(); 
        }
        ~CThreadPool()
        { 
            if (statues == Wait)
                Clear(); 
            ClearQueue(); 
            if (statues != Wait)
                Clear();
        }
    };

    template<typename CConnectionClass>
        class CConnectPool : public CPoolBase
    {
        private:
            std::map<CConnectionClass*, pthread_mutex_t> m_mapConnection;
            typename std::map<CConnectionClass*, pthread_mutex_t>::iterator m_iter;
            bool is_clear;
            typedef bool (CConnectionClass::*Connect)(const char*, const char*, const char*);
            typedef void (CConnectionClass::*Close)(void);
            Connect m_fconnect;
            Close m_fclose;
            char* m_sConnectParam[3];
            int m_iUsed;
            pthread_mutex_t m_mapmutex;
        public:
            CConnectPool(int num) : CPoolBase(num, Wait), m_iUsed(0)
        {
            is_clear = false;
            m_mapConnection.clear();
            m_sConnectParam[0] = NULL;
            m_sConnectParam[1] = NULL;
            m_sConnectParam[2] = NULL;
            pthread_mutex_init(&m_mapmutex, NULL);
        }
            ~CConnectPool()
            {
                Clear();
                free(m_sConnectParam[0]);
                free(m_sConnectParam[1]);
                free(m_sConnectParam[2]);
            }

            void Clear()
            {
                is_clear = true;
                m_iter = m_mapConnection.begin();
                while (m_iter != m_mapConnection.end())
                {
                    if (pthread_mutex_trylock(&m_iter->second) == 0)
                    {
                        (m_iter->first->*(m_fclose))();
                        delete m_iter->first;
                        m_mapConnection.erase(m_iter);
                        m_iter = m_mapConnection.begin();
                        pthread_mutex_lock(&m_mutex);
                        m_iCount --;
                        pthread_mutex_unlock(&m_mutex);
                    }
                    else m_iter++;
                    if (m_iter == m_mapConnection.end())
                        m_iter = m_mapConnection.begin();
                }
                m_mapConnection.clear();
            }

            void SetFunc(Connect connect, Close close, char* value[])
            {
                m_fconnect = connect;
                m_fclose = close;
                m_sConnectParam[0] = (char*)malloc(strlen(value[0]) + 1);
                m_sConnectParam[1] = (char*)malloc(strlen(value[1]) + 1);
                m_sConnectParam[2] = (char*)malloc(strlen(value[2]) + 1);
                strcpy(m_sConnectParam[0], value[0]);
                strcpy(m_sConnectParam[1], value[1]);
                strcpy(m_sConnectParam[2], value[2]);
            }

            CConnectionClass* GetConnection()
            {
                pthread_mutex_lock(&m_mapmutex);
                int count = 0;
                m_iter = m_mapConnection.begin();
                while (1)
                {
                    if (m_iter == m_mapConnection.end())
                    {
                        count ++;
                        m_iter = m_mapConnection.begin();
                        if (count > 10)
                        {
                            Create();
                            pthread_mutex_lock(&m_mutex);
                            m_iNum++;
                            pthread_mutex_unlock(&m_mutex);
                            m_iter = m_mapConnection.begin();
                        }
                    }
                    if (m_iter != m_mapConnection.end())
                        if (pthread_mutex_trylock(&m_iter->second) == 0)
                        {
                            pthread_mutex_lock(&m_mutex);
                            m_iUsed++;
                            pthread_mutex_unlock(&m_mutex);
                            pthread_mutex_unlock(&m_mapmutex);
                            return m_iter->first;
                        }
                        else m_iter++;
                }
                pthread_mutex_unlock(&m_mapmutex);
                return NULL;
            }

            void BackConnection(CConnectionClass* oneConnect)
            {
                if (!oneConnect)
                    return;
                pthread_mutex_lock(&m_mapmutex);
                pthread_mutex_unlock(&m_mapConnection.find(oneConnect)->second);
                pthread_mutex_unlock(&m_mapmutex);
                pthread_mutex_lock(&m_mutex);
                m_iUsed--;
                pthread_mutex_unlock(&m_mutex);
            }

            void Create()
            {
                pthread_mutex_t rwlock;
                pthread_mutex_init(&rwlock, NULL);
                pthread_mutex_lock(&m_mutex);
                CConnectionClass* oneConnect = new CConnectionClass();
                (oneConnect->*(m_fconnect))(m_sConnectParam[0], m_sConnectParam[1], m_sConnectParam[2]);
                m_mapConnection.insert(std::pair<CConnectionClass*, pthread_mutex_t> (oneConnect, rwlock));
                m_iCount ++;
                pthread_mutex_unlock(&m_mutex);
            }

            int  Count()
            {
                return m_mapConnection.size();
            }

            int  ControlThread(void* argv)
            {
                int in_count_add = 0;
                int in_count_del = 0;
                int task_count;
                int second = 10000;
                bool change;
                while(1)
                {
                    task_count = this->Count();
                    if (task_count <= m_iNum)
                    {
                        pthread_mutex_lock(&m_mapmutex);
                        Create();
                        pthread_mutex_unlock(&m_mapmutex);
                    }
                    if (m_iUsed > 0)
                        if (task_count)
                        {
                            if (task_count > m_iUsed)
                                in_count_del++;
                            if (in_count_del > 10)
                                change = true;
                        }
                    /*if (change && m_iNum > m_init)
                      {
                      if (in_count_del > 10)
                      {
                      in_count_del = 0;
                      in_count_add++;
                      pthread_mutex_lock(&m_mapmutex);
                      m_iter = m_mapConnection.begin();
                      while (m_iter != m_mapConnection.end())
                      {
                      if (pthread_mutex_trylock(&m_iter->second) == 0)
                      {
                      (m_iter->first->*(m_fclose))();
                      delete m_iter->first;
                      m_mapConnection.erase(m_iter);
                      m_iter = m_mapConnection.begin();
                      pthread_mutex_lock(&m_mutex);
                      m_iCount --;
                      pthread_mutex_unlock(&m_mutex);
                      break;
                      }
                      m_iter++;
                      }
                      pthread_mutex_unlock(&m_mapmutex);
                      }
                      if (in_count_add > 10)
                      {
                      m_init++;
                      in_count_add = 0;
                      }
                      }*/
                    if (task_count != 0)
                        usleep(second);

                    if (is_clear)
                        break;
                }
                return 0;
            }
    };

};

#endif
