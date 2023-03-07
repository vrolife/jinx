/*
MIT License

Copyright (c) 2023 pom@vro.life

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __jinx_buffer_hpp__
#define __jinx_buffer_hpp__

#include <cstdlib>
#include <cstring>

#include <atomic>
#include <tuple>

#include <jinx/async.hpp>
#include <jinx/meta.hpp>
#include <jinx/slice.hpp>
#include <jinx/linkedlist.hpp>
#include <jinx/error.hpp>
#include <jinx/result.hpp>
#include <jinx/pointer.hpp>
#include <jinx/optional.hpp>

namespace jinx {
namespace buffer {
namespace detail {
    
template<typename T, typename... Args>
struct is_memory_provider {
    template<typename C, typename = decltype(std::declval<C>().alloc(std::declval<Args>()...))>
    static std::true_type test(int);

    template<typename C>
    static std::false_type test(...);

    static constexpr bool value = decltype(test<T>(0))::value;
};

struct SumActiveBufferCount {
    size_t _sum{0};
    template<typename T>
    void operator()(T&& val) noexcept {
        _sum += val.active_buffer_count();
    }
};

struct SumReserveBufferCount {
    size_t _sum{0};
    template<typename T>
    void operator()(T&& val) noexcept {
        _sum += val.reserve_buffer_count();
    }
};

struct SumPendingRequestCount {
    size_t _sum{0};
    template<typename T>
    void operator()(T&& val) noexcept {
        _sum += val.pending_request_count();
    }
};
    
} // namespace detail

enum class ErrorBuffer {
    NoError,
    OutOfMemory
};

enum class ErrorAllocate {
    NoError,
    RegisterAllocateError,
    UnregisterAllocateError
};

JINX_ERROR_DEFINE(async_allocate, ErrorAllocate)
JINX_ERROR_DEFINE(buffer, ErrorBuffer)

class BufferView {
    char* _memory{nullptr};
    size_t _memory_size{0};
    size_t _begin{0};
    size_t _end{0};

public:
    BufferView() = default;
    BufferView(void* memory, size_t memory_size, size_t begin, size_t end)
    : _memory(reinterpret_cast<char*>(memory)), _memory_size{memory_size}, _begin{begin}, _end{end}
    { }
    
    ~BufferView() = default;
    BufferView(BufferView&&) = default;
    BufferView(const BufferView&) = default;
    BufferView& operator =(BufferView&&) = default;
    BufferView& operator =(const BufferView&) = default;

    void* memory() const noexcept {
        return _memory;
    }

    size_t memory_size() const noexcept {
        return _memory_size;
    }

    size_t capacity() const noexcept { return _memory_size - _end; }

    size_t size() const noexcept { return _end - _begin; }

    void reset_full() noexcept {
        _begin = 0;
        _end = _memory_size;
    }

    void reset_empty() noexcept {
        _begin = 0;
        _end = 0;
    }

    JINX_NO_DISCARD
    ResultGeneric resize(size_t size) noexcept {
        if (JINX_UNLIKELY(size > (_memory_size - _begin))) {
            return Failed_;
        }
        _end = _begin + size;
        return Successful_;
    }

    JINX_NO_DISCARD
    ResultGeneric cut(size_t n)  noexcept {
        if (n > _begin) {
            return Failed_;
        }
        _memory += n;
        _memory_size -= n;
        _begin -= n;
        _end -= n;
        return Successful_;
    }

    char* data() const noexcept { return &_memory[_begin]; }

    char* begin() const noexcept { return data(); }
    char* end() const noexcept { return &_memory[_end]; }

    JINX_NO_DISCARD
    ResultGeneric commit(size_t n) noexcept {
        if (JINX_UNLIKELY(n > capacity())) {
            return Failed_;
        }
        _end += n;
        return Successful_;
    }

    JINX_NO_DISCARD
    ResultGeneric consume(size_t n) noexcept {
        if (JINX_UNLIKELY(n > size())) {
            return Failed_;
        }
        _begin += n;
        return Successful_;
    }

    SliceConst slice_for_consumer() const noexcept {
        return { begin(), size() };
    }

    SliceMutable slice_for_producer() const noexcept {
        return { end(), capacity() };
    }
};

template<typename Pool, typename... Configs>
class BufferMemory final : private LinkedListThreadSafe<BufferMemory<Pool, Configs...>>::Node
{
    typedef pointer::PointerShared<BufferMemory<Pool, Configs...>> BufferType;
    typedef std::true_type SharedPointerRelease;
    typedef std::true_type SharedPointerGetPointer;
    typedef BufferView* PointerType;
    typedef jinx::Optional<std::tuple<typename Configs::Information...>> InfoTuple;

    friend LinkedListThreadSafe<BufferMemory<Pool, Configs...>>;
    friend BufferType;
    friend Pool;

    inline
    static void shared_pointer_release(BufferMemory* mem) {
        auto* pool = mem->_pool;
        pool->release(mem);
    }

    inline
    static
    BufferView* shared_pointer_get_pointer(BufferMemory* mem) {
        return &mem->_view;
    }

    void reset() noexcept {
        _info.reset();
        _info.emplace();
        _view = {_memory, _memory_size, 0, 0};
    }

public:
    explicit BufferMemory(Pool* pool, size_t memory_size)
    : _view{_memory, memory_size, 0, 0}, _pool(pool), _memory_size(memory_size)
    {
        _info.emplace();
    }

    ~BufferMemory() = default;
    JINX_NO_COPY_NO_MOVE(BufferMemory);

    BufferView& view() noexcept { return _view; }

    template<typename Config>
    inline
    typename Config::Information& info() noexcept {
        return std::get<meta::index_of<typename Config::Information, typename Configs::Information...>::value>(_info.get());
    }

    size_t memory_size() const noexcept { return _memory_size; }

private:
    std::atomic<long> _refc{0};

    BufferView _view;

    Pool* _pool;

    InfoTuple _info{};

    size_t _memory_size;
    alignas(void*) char _memory[0];
};

template<typename MemoryProvider, typename... Configs>
class BufferAllocator {
    static_assert(detail::is_memory_provider<MemoryProvider, size_t>::value, "First type must be a memory provider");

    template<typename Config, typename... Others>
    struct BufferPoolChain;

public:
    class BufferPool;
    class Allocate;

    typedef BufferMemory<BufferPool> MemoryType;
    typedef pointer::PointerShared<MemoryType> BufferType;

private:
    struct MemoryAllocator {
        MemoryProvider& _memory_provider;

        explicit MemoryAllocator(MemoryProvider& mem_provider):_memory_provider(mem_provider) { }

        JINX_NO_DISCARD
        Result<MemoryType*> allocate_memory(BufferPool* pool, size_t memory_size) noexcept
        {
            size_t size = sizeof(MemoryType) + memory_size + sizeof(void*);
            void* mem = _memory_provider.alloc(size);
            if (JINX_UNLIKELY(mem == nullptr)) {
                return nullptr;
            }
            new(mem)MemoryType(pool, memory_size);
            ::memcpy(reinterpret_cast<char*>(mem) + size - sizeof(void*), &mem, sizeof(void*));
            return reinterpret_cast<MemoryType*>(mem);
        }

        void deallocate_memory(MemoryType* memory) noexcept {
            const void* mem = memory;
            size_t size = sizeof(MemoryType) + memory->memory_size() + sizeof(void*);

            if (JINX_UNLIKELY(::memcmp(reinterpret_cast<const char*>(mem) + size - sizeof(void*), &mem, sizeof(const void*)) != 0)) {
                error::fatal("memory overflow");
            }
            
            memory->~BufferMemory();
            _memory_provider.free(memory);
        }
    };

    struct BufferPoolChainVoid {
        BufferPool _pool;
        MemoryAllocator& _allocator;

        explicit BufferPoolChainVoid(MemoryAllocator& allocator) 
        : _pool(allocator, 0, 0, -1),
          _allocator(allocator) { };
        
        BufferType allocate(size_t size) {
            return BufferType{_allocator.allocate_memory(&_pool, size)};
        }
        void get_pool() { }
        void reconfigure() { }
        void access(...) const { }
    };

    template<typename Config, typename... Others>
    class BufferPoolChain {
        typename std::conditional<sizeof...(Others) == 1, BufferPoolChainVoid, BufferPoolChain<Others...>>::
        type _next{};
        BufferPool _pool{};

    public:
        explicit BufferPoolChain(MemoryAllocator& memory_allocator)
        : _next(memory_allocator), 
          _pool(memory_allocator, Config::Size, Config::Reserve, Config::Limit)
        {
        }

        inline
        BufferPool* get_pool(const Config& ignored) noexcept {
            return &_pool;
        }

        template<typename C>
        inline
        BufferPool* get_pool(const C& ignored) noexcept {
            return _next.get_pool(ignored);
        }

        template<typename... Args>
        inline
        void reconfigure(const Config& ignored, Args&&... args) {
            _pool.reconfigure(std::forward<Args>(args)...);
        }

        template<typename C, typename... Args>
        inline
        void reconfigure(const C& ignored, Args&&... args) {
            _next.reconfigure(std::forward<Args>(args)...);
        }

        inline
        BufferType allocate(const Config& ignored) {
            return _pool.allocate();
        }

        template<typename C>
        inline
        BufferType allocate(const C& ignored) {
            return _next.allocate(ignored);
        }

        template<typename... Args>
        inline
        void access(Args&&... args) {
            _pool.access(std::forward<Args>(args)...);
            _next.access(std::forward<Args>(args)...);
        }

        template<typename... Args>
        inline
        void access(Args&&... args) const {
            _pool.access(std::forward<Args>(args)...);
            _next.access(std::forward<Args>(args)...);
        }
    };

    friend MemoryType;
    MemoryAllocator _memory_allocator;
    BufferPoolChain<Configs..., void> _pool_chain{};

public:
    explicit BufferAllocator(MemoryProvider& memory_provider)
    : _memory_allocator(memory_provider), _pool_chain(_memory_allocator)
    { };

    ~BufferAllocator() = default;
    JINX_NO_COPY_NO_MOVE(BufferAllocator);

    template<typename Config>
    BufferPool* get_pool(const Config& ignored) noexcept {
        return _pool_chain.get_pool(ignored);
    }

    template<typename Config>
    inline
    BufferType allocate(const Config& config) noexcept {
        return _pool_chain.allocate(config);
    }

    template<typename Config, typename... Args>
    inline
    void reconfigure(Args&&... args) {
        _pool_chain.reconfigure(Config{}, std::forward<Args>(args)...);
    }

    template<typename Callable>
    void access(Callable&& callable) {
        _pool_chain.template access(std::forward<Callable>(callable));
    }

    template<typename Callable>
    void access(Callable&& callable) const {
        _pool_chain.template access(std::forward<Callable>(callable));
    }

    size_t active_buffer_count() const {
        detail::SumActiveBufferCount sum{};
        access(sum);
        return sum._sum;
    }

    size_t reserve_buffer_count() const {
        detail::SumReserveBufferCount sum{};
        access(sum);
        return sum._sum;
    }

    size_t pending_request_count() const {
        detail::SumPendingRequestCount sum{};
        access(sum);
        return sum._sum;
    }
};

template<typename MemoryProvider, typename... Configs>
class BufferAllocator<MemoryProvider, Configs...>::BufferPool 
{
    MemoryAllocator &_memory_allocator;

    size_t _size{};
    size_t _reserve{};
    long _limit{};

    std::atomic<size_t> _active_buffers{0};
    
    LinkedListThreadSafe<MemoryType> _reserve_buffers;
    LinkedList<Allocate> _pending_requests;

    friend MemoryType;
    friend BufferAllocator<MemoryProvider, Configs...>;

    explicit BufferPool(MemoryAllocator& memory_allocator, size_t size, size_t reserve, size_t limit)
    : _memory_allocator(memory_allocator), _size(size), _reserve(reserve), _limit(limit)
    {
        reconfigure();
    }

    void reconfigure(size_t reserve, size_t limit) {
        _reserve = reserve;
        _limit = limit;

       this->reconfigure();
    }

    void reconfigure() {
        while (_reserve_buffers.get_size_unsafe() > _reserve) {
            auto* mem = _reserve_buffers.pop();
            release(mem);
        }
        while(_reserve_buffers.get_size_unsafe() < _reserve) {
            auto memory = _memory_allocator.allocate_memory(this, _size).unwrap();
            if (memory == nullptr) {
                break;
            }
            _active_buffers += 1;
            _reserve_buffers.push(memory);
        }
    }

    void release(MemoryType* memory) noexcept {
        if (not _pending_requests.empty()) 
        {
            auto refc = memory->_refc.fetch_add(1);
            if (refc != 0) {
                error::fatal("buffer use after free");
            }
            memory->reset();

            auto* request = _pending_requests.back();
            _pending_requests.erase(request) >> JINX_IGNORE_RESULT;

            // prevent unregister_request twice on *::async_finalize
            request->_pool = nullptr;

            request->emplace_result(BufferType{memory});
            request->async_resume() >> JINX_IGNORE_RESULT;

        } else {
            if (_reserve_buffers.get_size_unsafe() >= _reserve) {
                _memory_allocator.deallocate_memory(memory);
                _active_buffers -= 1;
            } else {
                _reserve_buffers.push(memory);
            }
        }
    }

    JINX_NO_DISCARD
    ResultGeneric register_request(Allocate* result) noexcept {
        return _pending_requests.push_back(result);
    }

    JINX_NO_DISCARD
    ResultGeneric unregister_request(Allocate* result) noexcept {
        return _pending_requests.erase(result);
    }

    template<typename Callable>
    void access(Callable&& callable) {
        callable(*this);
    }

    template<typename Callable>
    void access(Callable&& callable) const {
        callable(*this);
    }

public:

    BufferPool() = default;
    JINX_NO_COPY_NO_MOVE(BufferPool);
    ~BufferPool() {
        while (auto* mem = _reserve_buffers.pop()) {
            _memory_allocator.deallocate_memory(mem);
            _active_buffers -= 1;
        }
        jinx_assert(_active_buffers == 0 && "allocator destroyed before all buffers released");
    }

    size_t buffer_size() const noexcept { return _size; }

    size_t active_buffer_count() const noexcept {
        return _active_buffers;
    }

    size_t reserve_buffer_count() const noexcept {
        return _reserve_buffers.get_size_unsafe();
    }

    size_t pending_request_count() const noexcept {
        return _pending_requests.size();
    }

    BufferType allocate() noexcept {
        auto* memory = _reserve_buffers.pop();
        if (memory == nullptr) {
            if (_active_buffers > _limit) {
                return BufferType{nullptr};
            }
            memory = _memory_allocator.allocate_memory(this, _size).unwrap();
            if (memory == nullptr) {
                return BufferType{nullptr};
            }
            _active_buffers += 1;
        } else {
            memory->reset();
        }

        auto refc = memory->_refc.fetch_add(1);
        if (refc != 0) {
            error::fatal("buffer use after free");
        }
        return BufferType{memory};
    }
};

template<typename MemoryProvider, typename... Configs>
class BufferAllocator<MemoryProvider, Configs...>::Allocate
: public Awaitable, 
  public MixinResult<BufferType>, 
  public LinkedList<Allocate>::Node
{
    typedef BufferAllocator<MemoryProvider, Configs...> AllocatorType;

    friend AllocatorType;
    
    BufferPool* _pool{nullptr};

public:
    Allocate() = default;

    template<typename Config>
    Allocate& operator ()(AllocatorType* allocator, const Config& ignored) 
    {
        _pool = allocator->get_pool(Config{});
        this->reset();
        return *this;
    }

protected:
    void async_finalize() noexcept override {
        if (_pool != nullptr) {
            _pool->unregister_request(this) >> JINX_IGNORE_RESULT;
            _pool = nullptr;
        }
        Awaitable::async_finalize();
    }

    Async async_poll() override {
        if (not this->empty()) {
            return this->async_return();
        }

        auto buffer = _pool->allocate();
        if (buffer) {
            this->emplace_result(std::move(buffer));
            return this->async_return();
        }

        if (_pool->register_request(this).is(Failed_)) {
            return this->async_throw(ErrorAllocate::RegisterAllocateError);
        }
        return this->async_suspend();
    }
};

template<typename A>
class TaskBufferred : public Task
{
    void* _buffer{};
    DestroyTaskFunction _destroy{};
    A _awaitable{};

    explicit TaskBufferred(void* buffer, DestroyTaskFunction destroy)
    : _buffer(buffer), _destroy(destroy) { }

protected:
    DestroyTaskFunction destroy() override {
        return _destroy;
    }

public:
    template<typename... Args>
    TaskBufferred& operator()(Args&&... args) {
        _awaitable(std::forward<Args>(args)...);
        this->init(&_awaitable);
        return *this;
    }

    A* operator ->() noexcept {
        return &_awaitable;
    }

    template<typename Config, typename Allocator, typename... TArgs>
    JINX_NO_DISCARD
    static
    ResultGeneric allocate(TaskQueue* queue, Allocator* allocator, TArgs&&... args) {
        auto buf = allocator->allocate(Config{});
        if (buf == nullptr) {
            return Failed_;
        }

        auto& view = buf.get()->view();

        typedef TaskBufferred<A> TaskType;

        auto address = reinterpret_cast<uintptr_t>(view.memory());
        uintptr_t offset = address % alignof(TaskType);
        if (offset != 0) {
            if (view.cut(offset).is(Failed_)) {
                return Failed_;
            }
        }
        const auto size = sizeof(TaskType);
        jinx_assert(view.memory_size() >= size);
        
        auto* task = new(view.memory()) TaskType(
            buf.get(), 
            destroy_task<typename Allocator::BufferType, TaskType>
        );

        (*task)(std::forward<TArgs>(args)...);

        TaskPtr task_ptr{task};

        queue->spawn(task_ptr) >> JINX_IGNORE_RESULT;

        // self held buffer
        buf.release();
        
        return Successful_;
    }
private:
    template<typename Buffer, typename TaskType>
    static void destroy_task(Task* task) {
        auto* task_bufferred = static_cast<TaskType*>(task);
        
        // release later
        Buffer buf{reinterpret_cast<typename Buffer::ValueType*>(task_bufferred->_buffer)};

        task_bufferred->~TaskType();
    }
};

template<typename A>
struct TaskBufferredConfig {
    typedef TaskBufferred<A> TaskType;

    constexpr static char const* Name = "TaskBufferredConfig";
    static constexpr const size_t Size = sizeof(TaskBufferred<A>) + alignof(TaskBufferred<A>);;
    static constexpr const size_t Reserve = 10;
    static constexpr const long Limit = -1;
    struct Information { };
};

struct BufferConfigExample
{
    constexpr static char const* Name = "Example";
    static constexpr const size_t Size = 0x1000;
    static constexpr const size_t Reserve = 10;
    static constexpr const long Limit = -1;
    
    struct Information { };
};

} // namespace buffer
} // namespace jinx

#endif
