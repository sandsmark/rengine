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

#include <assert.h>
#include <vector>
#include <unordered_set>
#include <functional>
#include <algorithm>

RENGINE_BEGIN_NAMESPACE

template <typename ...Arguments> class Signal;

class SignalBase
{
public:
    virtual ~SignalBase() {}
};

template <typename ... Arguments>
class SignalHandler;
template<typename ... Arguments>
class SignalHandler_Function;

template <typename ... Arguments>
class SignalHandler
{
public:
    virtual ~SignalHandler() { }
    virtual void onSignal(Arguments ...args) = 0;

    static std::shared_ptr<SignalHandler_Function<Arguments...>> function(const std::function<void(Arguments ...)> handler) {
        return std::make_shared<SignalHandler_Function<Arguments...>>(handler);
    }
};

template<typename ... Arguments>
class SignalHandler_Function : public SignalHandler<Arguments ...>
{
public:
    using Ptr = std::shared_ptr<SignalHandler_Function<Arguments...>>;

    SignalHandler_Function(const std::function<void(Arguments ...)> handler) : m_handler(handler) { }
    void onSignal(Arguments ... args) override { m_handler(args...); }
private:
    std::function<void(Arguments ...)> m_handler;
};


class SignalEmitter
{
public:
    static Signal<> onDestruction;

    inline virtual ~SignalEmitter();

private:
    template <typename ...Arguments>
    friend class Signal;
    struct BucketBase
    {
        virtual ~BucketBase() { }
        SignalBase *signal;
    };
    std::vector<BucketBase *> *m_buckets = nullptr;
};


template <typename ...Arguments>
class Signal : SignalBase
{
    struct Bucket : public SignalEmitter::BucketBase
    {
        std::unordered_set<std::shared_ptr<SignalHandler<Arguments ...>>> handlers;
    };

public:
    void emit(SignalEmitter *emitter, Arguments ... args)
    {
        if (Bucket *bucket = findBucket(emitter)) {
            for (auto handler : bucket->handlers)
                handler->onSignal(args...);
        }
    }

    void connect(SignalEmitter *emitter, const std::function<void(Arguments ...)> function) {
        connect(emitter, SignalHandler<Arguments...>::function(function));
    }

    void connect(SignalEmitter *emitter, std::shared_ptr<SignalHandler<Arguments ...>> handler)
    {
        Bucket *bucket = findBucket(emitter);
        if (!bucket) {
            bucket = new Bucket();
            bucket->signal = this;
            if (!emitter->m_buckets) {
                emitter->m_buckets = new std::vector<SignalEmitter::BucketBase *>();
            }
            emitter->m_buckets->push_back(bucket);
        }
        bucket->handlers.insert(handler);
    }

    void disconnect(SignalEmitter *emitter, std::shared_ptr<SignalHandler<Arguments ...>> handler)
    {
        Bucket *bucket = findBucket(emitter);
        assert(bucket);
        bucket->handlers.erase(handler);
    }
private:

    Bucket *findBucket(SignalEmitter *emitter) const
    {
        if (!emitter->m_buckets)
            return 0;
        for (SignalEmitter::BucketBase *bucket : *emitter->m_buckets) {
            if (bucket->signal == this)
                return static_cast<Bucket *>(bucket);
        }
        return 0;
    }
};

SignalEmitter::~SignalEmitter()
{
    onDestruction.emit(this);
    if (m_buckets) {
        for (SignalEmitter::BucketBase *bucket : *m_buckets) {
            delete bucket;
        }
    }
    delete m_buckets;
}

#define RENGINE_SIGNALEMITTER_DEFINE_SIGNALS                                        \
    rengine::Signal<> rengine::SignalEmitter::onDestruction;                        \

RENGINE_END_NAMESPACE
