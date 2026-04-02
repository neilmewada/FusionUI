#pragma once

namespace Fusion
{
    
    template<typename T, SizeT GrowthIncrement = 128>
    class FStableDynamicArray
    {
        static_assert(GrowthIncrement > 0);
    public:

        static constexpr SizeT IncrementSize = GrowthIncrement;

        FStableDynamicArray()
        {}

        ~FStableDynamicArray()
        {
            Free();
        }

        FStableDynamicArray(const FStableDynamicArray& copy)
        {
            CopyFrom(copy);
        }

        FStableDynamicArray& operator=(const FStableDynamicArray& copy)
        {
            if (this == &copy)
                return *this;

            CopyFrom(copy);
            return *this;
        }

        FStableDynamicArray(FStableDynamicArray&& move) noexcept
        {
            data = move.data;
            count = move.count;
            capacity = move.capacity;

            move.data = nullptr;
            move.count = move.capacity = 0;
        }

        FStableDynamicArray& operator=(FStableDynamicArray&& move) noexcept
        {
            if (this == &move)
                return *this;

            Free();

            data = move.data;
            count = move.count;
            capacity = move.capacity;

            move.data = nullptr;
            move.count = move.capacity = 0;

            return *this;
        }

        typedef T* iterator;
        typedef const T* const_iterator;

        iterator begin() { return data; }
        const_iterator begin() const { return data; }
        iterator end() { return data + count; }
        const_iterator end() const { return data + count; }

        const T& operator[](SizeT index) const
        {
            return data[index];
        }

        T& operator[](SizeT index)
        {
            return data[index];
        }

        const T& First() const
        {
            return data[0];
        }

        T& First()
        {
            return data[0];
        }

        const T& Last() const
        {
            return data[count - 1];
        }

        T& Last()
        {
            return data[count - 1];
        }

        void Reserve(SizeT totalElementCapacity)
        {
            ZoneScoped;

	        if (capacity < totalElementCapacity)
	        {
                T* newData = new T[totalElementCapacity];
                if (data != nullptr)
                {
                    memcpy(newData, data, sizeof(T) * count);
                    delete[] data;
                }
                data = newData;
                
                capacity = totalElementCapacity;
	        }
        }

        void Free()
        {
            ZoneScoped;

	        if (data != nullptr)
	        {
                delete[] data;
	        }

            data = nullptr;
            capacity = count = 0;
        }

        inline void Grow()
        {
            ZoneScoped;

            Reserve(capacity + GrowthIncrement);
        }

        void Insert(const T& item)
        {
            ZoneScoped;

	        if (data == nullptr || count >= capacity)
	        {
                Grow();
	        }

            data[count++] = item;
        }

        void InsertRange(int numItems, const T& value = {})
        {
            ZoneScoped;

            if (data == nullptr || this->count + numItems > capacity)
	        {
                Reserve(capacity + std::max<SizeT>(this->count + numItems, GrowthIncrement));
	        }

            for (int i = 0; i < numItems; i++)
            {
                data[count++] = value;
            }
        }

        void Insert(T* values, int numItems)
        {
            ZoneScoped;

            if (data == nullptr || this->count + numItems > capacity)
            {
                Reserve(capacity + std::max<SizeT>(this->count + numItems, GrowthIncrement));
            }

            for (int i = 0; i < numItems; i++)
            {
                data[count++] = values[i];
            }
        }

        void RemoveAll()
        {
            ZoneScoped;

            count = 0;
        }

        void RemoveAt(SizeT index)
        {
            ZoneScoped;

            if (count == 0 || index >= count)
                return;

            for (SizeT i = index; i < count - 1; ++i)
            {
                data[i] = data[i + 1];
            }

            count--;
        }

        void RemoveLast()
        {
            RemoveAt(count - 1);
        }

        bool IsEmpty() const
        {
	        return count == 0 || data == nullptr;
        }

        SizeT GetCapacity() const
        {
            return capacity;
        }

        SizeT GetCount() const
        {
            return count;
        }

        SizeT GetByteSize() const
        {
            return count * sizeof(T);
        }

        T* GetData() const
        {
            return data;
        }
        
    private:

        void CopyFrom(const FStableDynamicArray& copy)
        {
            Free();

            Reserve(std::max(copy.capacity, copy.count));

            count = copy.count;
            
            if (count > 0)
            {
                memcpy(data, copy.data, count * sizeof(T));
            }
        }

        T* data = nullptr;
        SizeT capacity = 0;
        SizeT count = 0;
    };

} // namespace Fusion

