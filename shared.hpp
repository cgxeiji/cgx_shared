#pragma once

#include <optional>
#include <tuple>

namespace cgx::shared {

class lock_t {
   public:
    virtual void lock() noexcept = 0;
    virtual void unlock() noexcept = 0;
    virtual bool try_lock() noexcept = 0;

    virtual ~lock_t() = default;
};

template <typename T>
class smart_resource_t {
   public:
    smart_resource_t(lock_t& lock, T& resource)
        : m_lock(lock), m_resource(resource), m_is_locked(true) {
        m_lock.lock();
    }

    smart_resource_t(lock_t& lock, T& resource, bool try_lock)
        : m_lock(lock), m_resource(resource), m_is_locked(try_lock) {}

    ~smart_resource_t() {
        if (m_is_locked) {
            m_lock.unlock();
        }
    }

    smart_resource_t(const smart_resource_t&) = delete;
    smart_resource_t& operator=(const smart_resource_t&) = delete;
    smart_resource_t(smart_resource_t&& other) noexcept
        : m_lock(other.m_lock),
          m_resource(other.m_resource),
          m_is_locked(other.m_is_locked) {
        other.m_is_locked = false;  // avoid double unlock
    }
    smart_resource_t& operator=(smart_resource_t&& other) noexcept {
        if (this != &other) {
            if (m_is_locked) {
                m_lock.unlock();
            }
            m_lock = other.m_lock;
            m_resource = other.m_resource;
            m_is_locked = other.m_is_locked;
            other.m_is_locked = false;  // avoid double unlock
        }
        return *this;
    }

    bool is_locked() const noexcept { return m_is_locked; }

    T* operator->() { return &m_resource; }
    T& operator*() { return m_resource; }

   private:
    lock_t& m_lock;
    T& m_resource;
    bool m_is_locked;
};

template <typename T>
class resource_t {
   public:
    auto acquire() noexcept { return smart_resource_t(m_lock, m_resource); }

    std::optional<smart_resource_t<T>> try_acquire() noexcept {
        if (m_lock.try_lock()) {
            return smart_resource_t(m_lock, m_resource, true);
        }
        return std::nullopt;
    }

    resource_t(lock_t& lock, T& resource) noexcept
        : m_lock(lock), m_resource(resource) {}

    resource_t(const resource_t&) = delete;
    resource_t& operator=(const resource_t&) = delete;
    resource_t(resource_t&&) = delete;
    resource_t& operator=(resource_t&&) = delete;

   private:
    lock_t& m_lock;
    T& m_resource;
};

template <typename... ResourcesT>
class shared_resources_t {
   public:
    constexpr shared_resources_t(ResourcesT&... resources) noexcept
        : m_resources(resources...) {}

    template <typename T>
    auto acquire() {
        return std::get<resource_t<T>&>(m_resources).acquire();
    }

    template <typename T>
    auto try_acquire() {
        return std::get<resource_t<T>&>(m_resources).try_acquire();
    }

   private:
    std::tuple<ResourcesT&...> m_resources;
};

}  // namespace cgx::shared
