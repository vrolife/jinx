/*
Copyright (C) 2022  pom@vro.life

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef __lock_hpp__
#define __lock_hpp__

#include <mutex>

#include <jinx/async.hpp>

namespace jinx {
namespace lock {

class Lock;
class AsyncLock;

typedef pointer::PointerShared<Lock> LockPtr;

class Lock {
    friend pointer::PointerShared<Lock>;
    friend class AsyncLock;

    struct Wake : LinkedList<Wake>::Node {
        virtual void wake() = 0;
    };

    struct ThreadWake : Wake {
        std::mutex _mutex{};
        ThreadWake() {
            _mutex.lock();
        }
        JINX_NO_COPY_NO_MOVE(ThreadWake);
        ~ThreadWake() {
            _mutex.unlock();
        }

        void sleep() {
            _mutex.lock();
        }

        void wake() override {
            _mutex.unlock();
        }
    };

    std::atomic<long> _refc{1};
    std::atomic<long> _lock{0};
    LinkedList<Wake> _pending_aws{};
    std::mutex _mutex{};

public:
    Lock() = default;

    void lock() {
        _mutex.lock();
        if (_lock.fetch_add(1) == 0) {
            _mutex.unlock();
            return;
        }
        ThreadWake wake{};
        _pending_aws.push_front(&wake) >> JINX_IGNORE_RESULT;
        _mutex.unlock();
        wake.sleep();
    }

    void unlock() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_lock.fetch_sub(1) == 1) {
            return;
        }
        _pending_aws.front()->wake();
        _pending_aws.pop_front() >> JINX_IGNORE_RESULT;
    }
};

class AsyncLock : public Awaitable, private Lock::Wake
{
    friend class LinkedList<Lock::Wake>;

    LockPtr _lock{};

    Async (AsyncLock::* _fun)();

public:
    AsyncLock& operator()(LockPtr& lock) {
        _lock = lock;
        _fun = &AsyncLock::try_lock;
        return *this;
    }

protected:
    void wake() override {
        this->async_resume() >> JINX_IGNORE_RESULT;
    }

    void async_finalize() noexcept override {
        if (_lock != nullptr) {
            // suspended
            // unlock but not resume
            std::lock_guard<std::mutex> lock(_lock->_mutex);
            _lock->_lock.fetch_sub(1);
            _lock->_pending_aws.erase(this) >> JINX_IGNORE_RESULT;
        }
        _lock.reset();
        _fun = nullptr;
        Awaitable::async_finalize();
    }

    Async async_poll() override {
        return (this->*_fun)();
    }

    Async try_lock() {
        std::lock_guard<std::mutex> lock(_lock->_mutex);
        if (_lock->_lock.fetch_add(1) == 0) {
            _lock.reset();
            return this->async_return();
        }

        if (_lock->_pending_aws.push_front(this).is(Faileb)) {
            _lock.reset();
            return async_throw(ErrorAwaitable::InternalError);
        }
        _fun = &AsyncLock::locked;
        return this->async_suspend();
    }

    Async locked() {
        _lock.reset();
        return this->async_return();
    }
};

} // namespace lock
} // namespace jinx

#endif
