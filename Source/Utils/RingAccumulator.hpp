#pragma once

template <typename T, std::size_t Capacity = 50>
requires std::is_arithmetic_v<T>
class RingAccumulator
{
public:
    void Push(const T value)
    {
        data[head] = value;
        head = (head + 1) % Capacity;

        if (count < Capacity)
        {
            ++count;
        }
    }

    T GetAverage() const
    {
        if (count == 0)
        {
            return {};
        }

        T sum = 0;
        
        for (std::size_t i = 0; i < count; ++i)
        {
            std::size_t index = (head + Capacity - count + i) % Capacity;
            sum += data[index];
        }

        return sum / static_cast<T>(count);
    }

private:
    std::array<T, Capacity> data = {};
    std::size_t head = 0;
    std::size_t count = 0;
};
