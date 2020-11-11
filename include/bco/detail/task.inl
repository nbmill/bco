#pragma once

namespace bco {

namespace detail {

template <template <typename> typename _TaskT, typename T>
class PromiseType;

template <template <typename> typename _TaskT>
class PromiseType<_TaskT, void> {
    struct FinalAwaitable {
        bool await_ready() noexcept { return false; }
        //co_await await_suspend()���ڵ�ǰcoroutineִ�У�����ȥ��Ҳ�ǵ�ǰcoroutine_handle
        template <typename _PromiseT>
        std::coroutine_handle<void> await_suspend(std::coroutine_handle<_PromiseT> coroutine) noexcept
        {
            return coroutine.promise().caller_coroutine();
        }
        void await_resume() noexcept { }
        void return_void() { }
    };

public:
    //���캯�������в���������public
    PromiseType() = default;
    _TaskT<void> get_return_object()
    {
        return _TaskT<void> { std::coroutine_handle<PromiseType>::from_promise(*this) };
    }
    //ִ�е�coroutine��������ţ��ͻ�ִ��co_await initial_suspend()
    //���������������𣬸�coroutine��caller��ִ��co_await return_object
    //�Ա���coroutine��ʼִ��ǰ��ȡ���ⲿ��coroutine_handle
    std::suspend_always initial_suspend()
    {
        return {};
    }
    FinalAwaitable final_suspend()
    {
        return {};
    }
    std::coroutine_handle<> caller_coroutine()
    {
        return caller_coroutine_;
    }
    void unhandled_exception()
    {
    }
    void set_caller_coroutine(std::coroutine_handle<void> caller)
    {
        caller_coroutine_ = caller;
    }
    void return_void() { }
    void result() { }

private:
    std::coroutine_handle<> caller_coroutine_;
};

template <template <typename> typename _TaskT, typename T>
class PromiseType : PromiseType<_TaskT, void> {
public:
    _TaskT<T> get_return_object()
    {
        return _TaskT<void> { std::coroutine_handle<PromiseType>::from_promise(*this) };
    }
    void return_value(T t)
    {
        value_ = t;
    }
    T result()
    {
        return value_;
    }

private:
    T value_;
};

template <typename _PromiseT>
class Awaitable {
public:
    Awaitable(std::coroutine_handle<_PromiseT> coroutine)
        : current_coroutine_(coroutine)
    {
    }
    bool await_ready()
    {
        return false;
    }
    decltype(auto) await_resume() noexcept
    {
        return current_coroutine_.promise().result();
    }
    //����coroutine_handle<Task>�ƺ�Ҳ��
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller_coroutine)
    {
        current_coroutine_.promise().set_caller_coroutine(caller_coroutine);
        return current_coroutine_;
    }

private:
    std::coroutine_handle<_PromiseT> current_coroutine_;
};

} // namespace detail

} // namespace bco