#include <algorithm>

#include <bco/context.h>

namespace bco {

Context::Context(std::unique_ptr<ExecutorInterface>&& executor)
    : executor_ { std::move(executor) }
{
    executor_->set_proactor_task_getter(std::bind(&Context::get_proactor_tasks, this));
}

void Context::set_executor(std::unique_ptr<ExecutorInterface>&& executor)
{
    executor_ = std::move(executor);
    executor_->set_proactor_task_getter(std::bind(&Context::get_proactor_tasks, this));
}

ExecutorInterface* Context::executor()
{
    return executor_.get();
}

std::vector<PriorityTask> Context::get_proactor_tasks()
{
    std::vector<PriorityTask> tasks;
    for (auto& [_, proactor] : proactors_) {
        std::ranges::copy(proactor->harvest(), std::back_inserter(tasks));
    }
    return tasks;
}

void Context::start()
{
    executor_->set_context(weak_from_this());
    executor_->start();
}

void Context::spawn(std::function<Routine()>&& coroutine)
{
    executor_->post(PriorityTask { Priority::Medium, std::bind(&Context::spawn_aux, this, coroutine) });
}

void Context::add_routine(Routine routine)
{
    std::lock_guard lock { mutex_ };
    routines_.insert(RoutineInfo { routine, Clock::now() });
}

void Context::del_routine(Routine routine)
{
    std::lock_guard lock { mutex_ };
    constexpr auto _time = TimePoint {};
    routines_.erase(RoutineInfo { routine, _time });
}

size_t Context::routines_size()
{
    std::lock_guard lock { mutex_ };
    return routines_.size();
}

void Context::spawn_aux(std::function<Routine()> coroutine)
{
    coroutine();
}

} // namespace bco
