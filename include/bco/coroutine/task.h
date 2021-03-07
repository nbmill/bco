#pragma once
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <bco/utils.h>
#include "task.inl"

namespace bco {

template <typename T = void>
class Func;

template <typename T>
class Func {
public:
    friend class detail::PromiseType<Func, T>;
    using promise_type = detail::PromiseType<Func, T>;
    auto operator co_await()
    {
        return detail::Awaitable<promise_type> { coroutine_ };
    }

private:
    Func(std::coroutine_handle<promise_type> coroutine)
        : coroutine_(coroutine)
    {
    }
    std::coroutine_handle<promise_type> coroutine_;
};

class Routine : public std::suspend_never {
public:
    class promise_type {
    public:
        Routine get_return_object()
        {
            //coroutine_ = std::coroutine_handle<promise_type>::from_promise(*this);
            auto ctx = get_current_context().lock();
            if (ctx == nullptr) {
                return Routine {};
            } else {
                auto routine = Routine {};
                //TODO: ctx->add_routine(routine);
                return routine;
            }
        }
        std::suspend_never initial_suspend()
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            //TODO: 在此处告诉ContextBase我没了
            // ctx->del_routine();
            return {};
        }
        void unhandled_exception()
        {
        }
        void return_void()
        {
        }

    private:
        //std::coroutine_handle<promise_type> coroutine_;
        std::weak_ptr<bco::detail::ContextBase> ctx_;
    };
};

template <typename T = void>
class Task {
    struct SharedContext {
        std::optional<T> result_;
        std::coroutine_handle<> caller_coroutine_;
    };

public:
    Task()
        : ctx_(new SharedContext)
    {
    }
    void set_result(T&& val) noexcept { ctx_->result_ = std::move(val); }
    bool await_ready() const noexcept { return ctx_->result_.has_value(); }
    void await_suspend(std::coroutine_handle<> coroutine) noexcept
    {
        ctx_->caller_coroutine_ = coroutine;
    }
    T await_resume() noexcept
    {
        //return std::move(result_.value_or(detail::default_value<T>());
        //return ctx_->result_.value_or(detail::default_value<T>());
        return ctx_->result_.value_or(T {});
    }
    void resume() { ctx_->caller_coroutine_.resume(); }

protected:
    std::shared_ptr<SharedContext> ctx_;
};

template <>
class Task<void> {
    struct SharedContext {
        bool done_ = false;
        std::coroutine_handle<> caller_coroutine_;
    };

public:
    Task()
        : ctx_(new SharedContext)
    {
    }
    void set_done(bool done) noexcept { ctx_->done_ = done; }
    bool await_ready() const noexcept { return ctx_->done_; }
    std::function<void()> continuation()
    {
        return [this]() { ctx_->caller_coroutine_(); };
    }
    void await_suspend(std::coroutine_handle<> coroutine) noexcept
    {
        ctx_->caller_coroutine_ = coroutine;
    }
    void await_resume() noexcept { }
    void resume() { ctx_->caller_coroutine_.resume(); }

protected:
    std::shared_ptr<SharedContext> ctx_;
};

} //namespace bco