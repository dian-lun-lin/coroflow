#pragma once

#include "coro.hpp"

namespace taro { // begin of namespace taro ===================================

// ==========================================================================
//
// Task Traits
//
// ==========================================================================

class cudaWorker;

template <typename C>
constexpr bool is_static_task_v = 
  std::is_invocable_r_v<void, C> &&
  !std::is_invocable_r_v<Coro, C>;

template <typename C>
constexpr bool is_coro_task_v = 
  std::is_invocable_r_v<Coro, C>;
  //!std::is_invocable_r_v<void, C>; // TODO: why we cannot add this line?

template <typename T, typename>
struct get_index;

template <size_t I, typename... Ts>
struct get_index_impl {};

template <size_t I, typename T, typename... Ts>
struct get_index_impl<I, T, T, Ts...> : std::integral_constant<size_t, I>{};

template <size_t I, typename T, typename U, typename... Ts>
struct get_index_impl<I, T, U, Ts...> : get_index_impl<I+1, T, Ts...>{};

template <typename T, typename... Ts>
struct get_index<T, std::variant<Ts...>> : get_index_impl<0, T, Ts...>{};

template <typename T, typename... Ts>
constexpr auto get_index_v = get_index<T, Ts...>::value;

// ==========================================================================
//
// Decalartion of class Task
//
// Task stores a coroutine and handles dependencies of the task graph
// ==========================================================================

class Task {

  friend class Taro;
  friend class TaroV1;
  friend class TaroV2;
  friend class TaroV3;
  friend class TaroV4;
  friend class TaroV5;
  friend class TaroV6;
  friend class TaroV7;
  friend class TaroV8;
  friend class TaskHandle;

  struct CoroTask {
    template <typename C>
    CoroTask(C&&);

    std::function<taro::Coro()> work;
    Coro coro;
    void resume() {
      coro._resume();
    }

    bool done() {
      return coro._done();
    }
  };

  struct StaticTask {
    template <typename C>
    StaticTask(C&&);

    std::function<void()> work;
  };

  struct cudaTask {
    template <typename C>
    cudaTask(C&&);

    std::function<void(cudaWorker&)> work;
  };

  using handle_t = std::variant<
    std::monostate,
    CoroTask,
    StaticTask,
    cudaTask
  >;

  public:

    template <typename... Args>
    Task(size_t id, Args&&... args);


    Task() = default;
    ~Task() = default;

    // move construtor is for cudaTask
    // see cuda_suspend() in taro_v7
    // TODO: there must be a better implementation
    Task(Task&& rhs):_handle{std::move(rhs._handle)} {}
    Task& operator=(Task&& rhs) { _handle = std::move(rhs._handle);  return *this; }

    // coroutine should not be copied
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    constexpr static auto PLACEHOLDER   = get_index_v<std::monostate, handle_t>;
    constexpr static auto COROTASK   = get_index_v<CoroTask, handle_t>;
    constexpr static auto STATICTASK = get_index_v<StaticTask, handle_t>;
    constexpr static auto CUDATASK = get_index_v<cudaTask, handle_t>;

  private:

    void _precede(Task* task);
    std::vector<Task*> _succs;
    std::vector<Task*> _preds;
    std::atomic<int> _join_counter{0};
    size_t _id;

    handle_t _handle;
};

template <typename C>
Task::StaticTask::StaticTask(C&& c): work{std::forward<C>(c)} {
}

template <typename C>
Task::CoroTask::CoroTask(C&& c): 
  work{std::forward<C>(c)}, coro{work()}
{
}

template <typename C>
Task::cudaTask::cudaTask(C&& c): 
  work{std::forward<C>(c)} {
}

// ==========================================================================
//
// Definition of class Task
//
// ==========================================================================

template <typename... Args>
Task::Task(size_t id, Args&&... args):_id{id}, _handle{std::forward<Args>(args)...} {
}

void Task::_precede(Task* tp) {
  _succs.push_back(tp);
  tp->_preds.push_back(this);
  tp->_join_counter.fetch_add(1, std::memory_order_relaxed);
}

// ==========================================================================
//
// Decalartion of class TaskHandle
//
// ==========================================================================

class TaskHandle {

  public:

    TaskHandle();
    explicit TaskHandle(Task* tp);
    TaskHandle(TaskHandle&&) = default;
    TaskHandle(const TaskHandle&) = default;
    TaskHandle& operator = (const TaskHandle&) = default;
    TaskHandle& operator = (TaskHandle&&) = default;
    ~TaskHandle() = default;    

    TaskHandle& precede(TaskHandle ch);

    TaskHandle& succeed(TaskHandle ch);

  private:

    Task* _tp;
};

// ==========================================================================
//
// Definition of class TaskHandle
//
// ==========================================================================
//
TaskHandle::TaskHandle(): _tp{nullptr} {
}

TaskHandle::TaskHandle(Task* tp): _tp{tp} {
}

TaskHandle& TaskHandle::precede(TaskHandle ch) {
  _tp->_precede(ch._tp);
  return *this;
}

TaskHandle& TaskHandle::succeed(TaskHandle ch) {
  ch._tp->_precede(_tp);
  return *this;
}


} // end of namespace taro ==============================================
