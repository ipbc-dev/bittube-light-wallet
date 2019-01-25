//
// Created by mwo on 11/07/18.
//

#ifndef OPENMONERO_PINGTHREAD_H
#define OPENMONERO_PINGTHREAD_H

#include <thread>

#include "ctpl.h"

namespace xmreg
{

// based on Mayer's ThreadRAII class (item 37)
class ThreadRAII
{
public:
    enum class DtorAction { join, detach};

    ThreadRAII(std::thread&& _t, DtorAction _action);

    ThreadRAII(ThreadRAII&&) = default;
    ThreadRAII& operator=(ThreadRAII&&) = default;

    std::thread& get() {return t;}

    ~ThreadRAII();

private:
    std::thread t;
    DtorAction action;
};

ctpl::thread_pool& getTxSearchPool();

template <typename T>
class ThreadRAII2 {
public:
    ThreadRAII2(std::unique_ptr<T> _functor) : f{std::move(_functor)}
    {
	    getTxSearchPool().push([this](int thread_idx) {
			this->get_functor().search();
	    });
    };

    ThreadRAII2(ThreadRAII2&&) = default;
    ThreadRAII2& operator=(ThreadRAII2&&) = default;

    T& get_functor() { return *f; }

protected:
    std::unique_ptr<T> f;
};

}

#endif //OPENMONERO_PINGTHREAD_H
