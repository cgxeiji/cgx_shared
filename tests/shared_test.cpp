#include "../shared.hpp"

#include <gtest/gtest.h>

#include <mutex>
#include <optional>
#include <thread>
#include <vector>

class mock_lock_t : public cgx::shared::lock_t {
   public:
    void lock() noexcept override {
        mutex.lock();
        locked = true;
    }

    void unlock() noexcept override {
        locked = false;
        mutex.unlock();
    }

    bool try_lock() noexcept override {
        if (mutex.try_lock()) {
            locked = true;
            return true;
        }
        return false;
    }

    bool is_locked() const noexcept { return locked; }

   private:
    std::mutex mutex;
    bool locked = false;
};

TEST(ResourceTest, TryLockSuccess) {
    mock_lock_t lock;
    int resource = 100;
    cgx::shared::resource_t<int> res(lock, resource);

    auto smart_res = res.try_acquire();
    ASSERT_TRUE(smart_res);
    EXPECT_EQ(**smart_res, 100);
    EXPECT_TRUE(lock.is_locked());
}

TEST(ResourceTest, TryLockFailure) {
    mock_lock_t lock;
    int resource = 100;
    cgx::shared::resource_t<int> res(lock, resource);

    lock.lock();  // Manually lock the resource to simulate failure
    auto smart_res = res.try_acquire();
    ASSERT_FALSE(smart_res);
    EXPECT_TRUE(lock.is_locked());
    lock.unlock();  // Clean up
}

TEST(ResourceTest, MultiThreadAccess) {
    mock_lock_t lock;
    int resource = 100;
    cgx::shared::resource_t<int> res(lock, resource);

    const int num_threads = 5;
    std::vector<std::thread> threads;
    std::vector<int> access_order;
    std::mutex order_mutex;

    auto thread_func = [&](int thread_id) {
        auto smart_res = res.try_acquire();
        if (smart_res) {
            std::lock_guard<std::mutex> lg(order_mutex);
            access_order.push_back(thread_id);
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));  // Simulate work
        }
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_func, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(access_order.size(),
              1);  // Only one thread should acquire the lock
    EXPECT_EQ(access_order.front(),
              0);  // The first thread should acquire the lock
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
