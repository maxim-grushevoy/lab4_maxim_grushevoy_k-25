#pragma once
// Структура даних для варіанту №11: m = 3 цілих поля.
// Захист: по одному std::shared_mutex на КОЖНЕ поле (разом 3 м'ютекси).
#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <string>

#ifdef USE_PLAIN_MUTEX  
using FieldMutex = std::mutex;
using ReadLock   = std::lock_guard<std::mutex>;
#else
using FieldMutex = std::shared_mutex;
using ReadLock   = std::shared_lock<std::shared_mutex>;
#endif

class GuardedFields
{
public:
    static constexpr std::size_t M = 3;

    GuardedFields() = default;
    GuardedFields(const GuardedFields &) = delete;
    GuardedFields &operator=(const GuardedFields &) = delete;
    ~GuardedFields() = default;

    int get(std::size_t i) const
    {
        ReadLock lk(slots_[i].mtx);
        return slots_[i].value;
    }

    void set(std::size_t i, int v)
    {
        std::unique_lock<FieldMutex> lk(slots_[i].mtx);
        slots_[i].value = v;
    }

    explicit operator std::string() const
    {
        int snap[M];
        {
            ReadLock l0(slots_[0].mtx);
            ReadLock l1(slots_[1].mtx);
            ReadLock l2(slots_[2].mtx);
            for (std::size_t i = 0; i < M; ++i)
                snap[i] = slots_[i].value;
        }
        std::string s = "[";
        for (std::size_t i = 0; i < M; ++i)
        {
            if (i) s += ", ";
            s += std::to_string(snap[i]);
        }
        s += "]";
        return s;
    }

private:
    struct alignas(64) Slot
    {
        mutable FieldMutex mtx;
        int value = 0;
    };
    Slot slots_[M]; 
};
