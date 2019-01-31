//
// Created by mwo on 11/07/18.
//

#ifndef OPENMONERO_THREADRAII_H
#define OPENMONERO_THREADRAII_H

#include <thread>
#include <iostream>

#include "ctpl.h"

namespace xmreg
{

// based on Mayer's ThreadRAII class (item 37)
/*
class ThreadRAII
{
public:
    enum class DtorAction {join, detach};

    ThreadRAII(std::thread&& _t, DtorAction _action);

    ThreadRAII(ThreadRAII&&) = default;
    ThreadRAII& operator=(ThreadRAII&&) = default;

    virtual std::thread& get() {return t;}

    virtual ~ThreadRAII();

protected:
    std::thread t;
    DtorAction action;
};

template <typename T>
class ThreadRAII2 : public ThreadRAII
{
public:

    ThreadRAII2(std::unique_ptr<T> _functor,
                DtorAction _action = DtorAction::join)
        :ThreadRAII(std::thread(std::ref(*_functor)), _action),
         f {std::move(_functor)}
    {}

    T& get_functor() {return *f;}

protected:
    std::unique_ptr<T> f;
};
*/


ctpl::thread_pool& getTxSearchPool();

template <typename T>
class ThreadRAII2 {
public:
    ThreadRAII2(std::unique_ptr<T> _functor) : f{std::move(_functor)}
    {
	    getTxSearchPool().push([this](int thread_idx) {
			(this->get_functor())();
	    });
    };

    ThreadRAII2(ThreadRAII2&&) = default;
    ThreadRAII2& operator=(ThreadRAII2&&) = default;

    T& get_functor() { return *f; }

protected:
    std::unique_ptr<T> f;
};

}

#endif //OPENMONERO_THREADRAII_H
