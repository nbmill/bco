#include <bco/coroutine/task.h>
#include <bco/context.h>

namespace bco {

Routine Routine::promise_type::get_return_object()
{
    auto ctx = get_current_context().lock();
    if (ctx == nullptr) {
        return Routine { this };
    } else {
        //�����Ƚ�ctx���浽ctx_����Ϊ�����add_routine()����ֵ������ӹ�ȥ��
        ctx_ = ctx;
        auto routine = Routine { this };
        ctx->add_routine(routine);
        return routine;
    }
}

std::suspend_never Routine::promise_type::final_suspend() noexcept
{
    if (auto ctx = ctx_.lock()) {
        ctx->del_routine(Routine { this });
    }
    return {};
}

} // namespace bco
