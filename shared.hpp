#pragma once

#include <tuple>

namespace cgx::shared {

class lock_t {
   public:
    virtual void lock() = 0;
    virtual void unlock() = 0;

    // virtual bool is_locked() = 0;
    // bool try_lock() {
    //     if (is_locked()) {
    //         return false;
    //     }
    //     lock();
    //     return true;
    // }

    virtual ~lock_t() = default;
};

template <typename T>
class smart_resource_t {
   public:
    smart_resource_t(lock_t& lock, T& resource)
        : m_lock(lock), m_resource(resource) {
        m_lock.lock();
    }

    ~smart_resource_t() { m_lock.unlock(); }

    T* operator->() { return &m_resource; }
    T& operator*() { return m_resource; }

   private:
    lock_t& m_lock;
    T& m_resource;
};

template <typename T>
class resource_t {
   public:
    auto acquire() { return smart_resource_t(m_lock, m_resource); }

    resource_t(lock_t& lock, T& resource)
        : m_lock(lock), m_resource(resource) {}

   private:
    lock_t& m_lock;
    T& m_resource;
};

template <typename... ResourcesT>
class shared_resources_t {
   public:
    shared_resources_t(ResourcesT&... resources) : m_resources(resources...) {}

    template <typename T>
    auto acquire() {
        return std::get<resource_t<T>&>(m_resources).acquire();
    }

   private:
    std::tuple<ResourcesT&...> m_resources;
};

}  // namespace cgx::shared
