
#include <iostream>

#define __cpp_impl_coroutine 1
# include <coroutine>
#undef  __cpp_impl_coroutine
namespace std::inline experimental
{
    using namespace std;
}

inline namespace aux
{
    template <class T>
    struct task {
        struct promise_type {
            auto get_return_object() {
                return task(std::coroutine_handle<promise_type>::from_promise(*this));
            }
            auto initial_suspend() {
                return std::suspend_always{};
            }
            struct final_awaiter {
                bool await_ready() noexcept {
                    return false;
                }
                void await_resume() noexcept {
                }
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> handle) noexcept
                {
                    /*
                      final_awaiter::await_suspend is called when the execution of the
                      current coroutine (referred to by 'handle') is about to finish.
                      If the current coroutine was resumed by another coroutine via
                      co_await get_task(), a handle to that coroutine has been stored
                      as handle.promise().previous. In that case, return the handle to resume 
                      the previous coroutine.
                      Otherwise, return noop_coroutine(), whose resumption does nothing.
                    */
                    auto previous = handle.promise().previous;
                    if (previous) {
                        return previous;
                    }
                    else {
                        return std::noop_coroutine();
                    }
                }
            };
            auto final_suspend() noexcept {
                return final_awaiter{};
            }
            void unhandled_exception() {
                throw;
            }
            void return_value(T value) {
                this->result = std::move(value);
            }
            T result;
            std::coroutine_handle<> previous;
        };
        task(task&& t) = default;
        task(std::coroutine_handle<promise_type> handle)
            : continuation{handle}
        {
        }
        ~task() {
            this->continuation.destroy();
        }
        struct awaiter {
            bool await_ready() {
                return false;
            }
            T await_resume() {
                return std::move(this->continuation.promise().result);
            }
            auto await_suspend(std::coroutine_handle<> handle) {
                this->continuation.promise().previous = handle;
                return this->continuation;
            }
            std::coroutine_handle<promise_type> continuation;
        };
        awaiter operator co_await() {
            return awaiter{this->continuation};
        }
        T operator()() {
            this->continuation.resume();
            return std::move(this->continuation.promise().result);
        }
    private:
        std::coroutine_handle<promise_type> continuation;
    };
} // ::aux

task<int> get_random() {
    std::cout << "in get_random()\n";
    co_return 4;
}

task<int> test() {
    task<int> v = get_random();
    task<int> u = get_random();
    std::cout << "in test()\n";
    int x = (co_await v + co_await u);
    co_return x;
}

int main() {
    task<int> t = test();
    int result = t();
    std::cout << result << '\n';
    return 0;
}
