/*******************************************************************************
 * Author : huwq
 * Email : sohunjug@hotmail.com
 * Last modified : 2013-11-18 10:46
 * Filename : CTaskQueue.h
 * Description : 
 * *****************************************************************************/
#ifndef __TASH_QUEUE_H__
#define __TASH_QUEUE_H__
#include <queue>
#include <vector>
#include <pthread.h>
#include <new>

namespace QUEUE
{
    enum Priority
    {
        Urgent = 1,
        Highest,
        High,
        Middle,
        Low,
        Minimum
    };

    class CTaskBase
    {
        public:
            Priority priority;
            CTaskBase()
            {
                is_virtual = 0;
                argv = NULL;
            }
            virtual ~CTaskBase(){}

            virtual int Start()
            {
                return 0;
            }

            bool operator () (CTaskBase* &a, CTaskBase* &b) 
            {
                return !((*a) < (*b));
            }

            bool operator () (CTaskBase &a, CTaskBase &b) 
            {
                return !(a < b);
            }
            bool operator < (const CTaskBase &a) const
            {
                return (bool)((int)priority < (int)a.priority);
            }

        protected:
            void *argv;
            int  is_virtual;
    };

    template <class Type>
        class CTaskQueue : public CTaskBase
    {
        public:
            union
            {
                int (Type::*func)(void*);
                int (*CallBack)(Type*, void*);
            }func;
            CTaskQueue()
            {
                ClassName = NULL;
                func.func = NULL;
            }
            ~CTaskQueue()
            {
                Clear();
            }

            void Clear()
            {
                argv = NULL;
                ClassName = NULL;
                func.func = NULL;
            }

            int SetFunc(Priority priority, Type* ClassName, int (Type::*Func)(void*), void* argv)
            {
                this->priority = priority;
                this->ClassName = ClassName;
                this->argv = argv;
                this->func.func = Func;
                return 1;
            }
            int Start()
            {
                //return func.CallBack(ClassName, argv);
                return (ClassName->*(func.func))(argv);
            }

        private:
            Type* ClassName;

    };

    template<typename _Tp, typename _Sequence = std::vector<_Tp>,
        typename _Compare  = std::less<typename _Sequence::value_type> >
            class priority_queue
            {
                // concept requirements
                pthread_rwlock_t m_mutex;

                public:
                typedef typename _Sequence::value_type                value_type;
                typedef typename _Sequence::reference                 reference;
                typedef typename _Sequence::const_reference           const_reference;
                typedef typename _Sequence::size_type                 size_type;
                typedef          _Sequence                            container_type;

                protected:
                //  See queue::c for notes on these names.
                _Sequence  c;
                _Compare   comp;

                public:
                explicit priority_queue(const _Compare& __x = _Compare(), const _Sequence& __s = _Sequence())
                    : c(__s), comp(__x)
                { 
                    pthread_rwlock_init(&m_mutex, NULL);
                    std::make_heap(c.begin(), c.end(), comp); 
                }

                template<typename _InputIterator>
                    priority_queue(_InputIterator __first, _InputIterator __last, const _Compare& __x = _Compare(), const _Sequence& __s = _Sequence())
                    : c(__s), comp(__x)
                    {
                        pthread_rwlock_init(&m_mutex, NULL);
                        __glibcxx_requires_valid_range(__first, __last);
                        c.insert(c.end(), __first, __last);
                        std::make_heap(c.begin(), c.end(), comp);
                    }

                bool empty() 
                { 
                    pthread_rwlock_wrlock(&m_mutex);
                    bool e = c.empty(); 
                    pthread_rwlock_unlock(&m_mutex);
                    return e;
                }

                size_type size() 
                { 
                    pthread_rwlock_wrlock(&m_mutex);
                    size_type _size = c.size(); 
                    pthread_rwlock_unlock(&m_mutex);
                    return _size;
                }

                const_reference top() 
                {
                    pthread_rwlock_wrlock(&m_mutex);
                    const_reference reference_value = c.front();
                    return reference_value;
                }

                void push(const value_type& __x)
                {
                    pthread_rwlock_wrlock(&m_mutex);
                    c.push_back(__x);
                    std::push_heap(c.begin(), c.end(), comp);
                    pthread_rwlock_unlock(&m_mutex);
                }
                void pop()
                {
                    std::pop_heap(c.begin(), c.end(), comp);
                    c.pop_back();
                    pthread_rwlock_unlock(&m_mutex);
                }

            };
    typedef priority_queue<CTaskBase*, std::vector<CTaskBase*>, CTaskBase> queue_priority;

};

#endif
