#pragma once
// Active implementation of recursive_generator<T> for C++20 builds.
// Replaces the cppcoro C++17/experimental path (archived as cppcoro.ARCHIVE/).
// Include via qtl/generator.h, not directly.
#include <coroutine>
#include <cassert>

// C++20 recursive_generator — direct port of cppcoro's m_root/m_parentOrLeaf design.
// Correctly handles: co_yield value; co_yield nested_generator;

template<typename T>
struct recursive_generator {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        T*            m_value      = nullptr;
        promise_type* m_root;        // always points to the root promise
        promise_type* m_parentOrLeaf;// root: current leaf; non-root: parent

        promise_type() noexcept : m_root(this), m_parentOrLeaf(this) {}
        promise_type(const promise_type&) = delete;
        promise_type(promise_type&&)      = delete;

        recursive_generator get_return_object() noexcept {
            return recursive_generator{ handle_type::from_promise(*this) };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void()         noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }

        // yield a plain value
        std::suspend_always yield_value(T& value) noexcept {
            m_value = std::addressof(value);
            return {};
        }
        std::suspend_always yield_value(T&& value) noexcept {
            m_value = std::addressof(value);
            return {};
        }

        // yield a nested generator
        auto yield_value(recursive_generator& gen) noexcept {
            struct awaitable {
                promise_type* m_childPromise;
                bool await_ready()              noexcept { return m_childPromise == nullptr; }
                void await_suspend(handle_type) noexcept {}
                void await_resume()             noexcept {}
            };

            if (gen.m_promise != nullptr) {
                m_root->m_parentOrLeaf        = gen.m_promise;
                gen.m_promise->m_root         = m_root;
                gen.m_promise->m_parentOrLeaf = this;
                gen.m_promise->resume();        // start child immediately

                if (!gen.m_promise->is_complete()) {
                    return awaitable{ gen.m_promise };
                }
                // child was empty — restore leaf pointer
                m_root->m_parentOrLeaf = this;
            }
            return awaitable{ nullptr };
        }
        auto yield_value(recursive_generator&& gen) noexcept { return yield_value(gen); }

        bool is_complete() noexcept {
            return handle_type::from_promise(*this).done();
        }

        // called only on root; returns current leaf's value
        T& value() noexcept {
            assert(this == m_root);
            assert(!is_complete());
            return *(m_parentOrLeaf->m_value);
        }

        // called only on root; advance to next value
        void pull() noexcept {
            assert(this == m_root);
            m_parentOrLeaf->resume();
            while (m_parentOrLeaf != this && m_parentOrLeaf->is_complete()) {
                m_parentOrLeaf = m_parentOrLeaf->m_parentOrLeaf;
                m_parentOrLeaf->resume();
            }
        }

    private:
        void resume() noexcept {
            handle_type::from_promise(*this).resume();
        }
    };

    // ── generator object ──────────────────────────────────────────────────────
    promise_type* m_promise;

    recursive_generator() noexcept : m_promise(nullptr) {}

    explicit recursive_generator(handle_type h) noexcept
        : m_promise(std::addressof(h.promise())) {}

    ~recursive_generator() {
        if (m_promise)
            handle_type::from_promise(*m_promise).destroy();
    }

    recursive_generator(const recursive_generator&) = delete;
    recursive_generator& operator=(const recursive_generator&) = delete;

    recursive_generator(recursive_generator&& o) noexcept
        : m_promise(std::exchange(o.m_promise, nullptr)) {}

    recursive_generator& operator=(recursive_generator&& o) noexcept {
        if (this != &o) {
            if (m_promise)
                handle_type::from_promise(*m_promise).destroy();
            m_promise = std::exchange(o.m_promise, nullptr);
        }
        return *this;
    }

    // ── iterator ──────────────────────────────────────────────────────────────
    struct iterator {
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::remove_reference_t<T>;
        using reference         = T&;
        using pointer           = T*;

        promise_type* m_promise;

        iterator() noexcept : m_promise(nullptr) {}
        explicit iterator(promise_type* p) noexcept : m_promise(p) {}

        bool operator==(const iterator& o) const noexcept { return m_promise == o.m_promise; }
        bool operator!=(const iterator& o) const noexcept { return !(*this == o); }

        reference operator*()  const noexcept { return m_promise->value(); }
        pointer   operator->() const noexcept { return std::addressof(**this); }

        iterator& operator++() {
            assert(m_promise != nullptr);
            assert(!m_promise->is_complete());
            m_promise->pull();
            if (m_promise->is_complete())
                m_promise = nullptr;
            return *this;
        }
        void operator++(int) { (void)operator++(); }
    };

    iterator begin() {
        if (m_promise != nullptr) {
            m_promise->pull();
            if (!m_promise->is_complete())
                return iterator{ m_promise };
        }
        return iterator{ nullptr };
    }

    iterator end() noexcept { return iterator{ nullptr }; }
};
