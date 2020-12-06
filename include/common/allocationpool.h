/*
    Copyright (c) 2015, Gunnar Sletta <gunnar@sletta.org>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "common/common.h"

#include <assert.h>
#include <iostream>
#include <memory>

RENGINE_BEGIN_NAMESPACE

template <typename T>
class AllocationPool
{
public:
    typedef T value_type;

    AllocationPool() :
        m_pool(std::make_shared<Pool>())
    {
    }
    AllocationPool(const AllocationPool &other) : m_pool(other.m_pool) {}

    /*!
     * For internal use only, called by setup macroes.
     */
    void setMemory(void *m, unsigned blockCount) {
        assert(m_pool);
        assert(m_pool->memory == 0);
        assert(m_pool->free == 0);
        assert(m_pool->nextFree == 0);
        assert(m_pool->size == 0);

        m_pool->size = blockCount;
        m_pool->memory = (T *) m;
        m_pool->free = (unsigned *) (m_pool->memory + blockCount);
        m_pool->nextFree = 0;
        for (unsigned i=0; i<blockCount; ++i) {
            m_pool->free[i] = i;
        }
    }

    T *allocate() {
        assert(!isExhausted());
        assert(m_pool->memory);
        assert(m_pool->free);
        assert(m_pool->free[m_pool->nextFree] < m_pool->size);

        // std::cout << "AllocationPool::allocate: memory=" << m_memory
        //           << ", free=" << m_free
        //           << ", nextFree=" << m_nextFree
        //           << ", free[nextFree]=" << m_free[m_nextFree] << std::endl;

        T *t = m_pool->memory + m_pool->free[m_pool->nextFree++];
        // std::cout << " --> t=" << (void *) t
        //           << ", nextFree=" << m_nextFree
        //           << ", free[nextFree]=" << m_free[m_nextFree] << std::endl;

        return new (t) T();
    }

    void deallocate(T *t) {
        assert(m_pool->memory);
        assert(m_pool->free);
        assert(m_pool->nextFree > 0);
        assert(isAlloctated(t));

        unsigned pos = t - m_pool->memory;

        --m_pool->nextFree;
        m_pool->free[m_pool->nextFree] = pos;

        // Call the destructor...
        t->~T();
    }

    bool isExhausted() const { return m_pool->nextFree >= m_pool->size; }
    bool isAlloctated(T *t) const { return unsigned(t - m_pool->memory) < m_pool->size; }

private:
    struct Pool {
        T *memory = nullptr;
        unsigned *free = nullptr;
        unsigned nextFree = 0u;
        unsigned size = 0u;
    };
    std::shared_ptr<Pool> m_pool;
};

#define RENGINE_ALLOCATION_POOL(Type, Name, Count) \
    Type::__allocation_pool_##Name.setMemory(alloca(Count * (sizeof(Type) + sizeof(unsigned))), Count)

#define RENGINE_ALLOCATION_POOL_DECLARATION(Type, Name)     \
    friend class AllocationPool<Type>;                      \
    static AllocationPool<Type> __allocation_pool_##Name;   \
    static Type *create() {                                 \
        if (__allocation_pool_##Name.isExhausted())         \
            return new Type();                              \
        else  {                                             \
            Type *t = __allocation_pool_##Name.allocate();  \
            t->__mark_as_pool_allocated();                  \
            return t;                                       \
        }                                                   \
    }                                                       \
    virtual void destroy() override {                       \
        if (__is_pool_allocated())                          \
            __allocation_pool_##Name.deallocate(this);      \
        else                                                \
            delete this;                                    \
    }

#define RENGINE_ALLOCATION_POOL_DECLARATION_IN_BASECLASS(Type, Name)     \
    friend class AllocationPool<Type>;                                   \
    static AllocationPool<Type> __allocation_pool_##Name;                \
    static Type *create() {                                              \
        if (__allocation_pool_##Name.isExhausted())                      \
            return new Type();                                           \
        else  {                                                          \
            Type *t = __allocation_pool_##Name.allocate();               \
            t->__mark_as_pool_allocated();                               \
            return t;                                                    \
        }                                                                \
    }                                                                    \
    virtual void destroy() {                                             \
        if (__is_pool_allocated())                                       \
            __allocation_pool_##Name.deallocate(this);                   \
        else                                                             \
            delete this;                                                 \
    }

#define RENGINE_ALLOCATION_POOL_DEFINITION(Type, Name)      \
    rengine::AllocationPool<Type> Type::__allocation_pool_##Name


RENGINE_END_NAMESPACE
