#pragma once

#include "unordered_dense.h"

namespace Fusion
{

    template<typename KeyType, typename ValueType>
    class TPair
    {
    public:

        constexpr TPair() : first({}), second({})
        {

        }

        constexpr TPair(KeyType key, ValueType value) : first(key), second(value)
        {

        }

        KeyType first;
        ValueType second;

        SizeT GetHash() const
        {
            SizeT hash = ::Fusion::GetHash<KeyType>(first);
            CombineHash(hash, second);
            return hash;
        }

        bool operator==(const TPair& rhs) const
        {
            return GetHash() == rhs.GetHash();
        }

        bool operator!=(const TPair& rhs) const
        {
            return !operator==(rhs);
        }
    };

    template<typename KeyType, typename ValueType>
    class THashMap
    {
    public:
        THashMap() : Impl({})
        {

        }

        THashMap(std::initializer_list<std::pair<KeyType, ValueType>> list) : Impl(list)
        {

        }

        ~THashMap()
        {

        }

        inline SizeT GetSize() const
        {
            return Impl.size();
        }

        inline bool IsEmpty() const
        {
            return Impl.empty();
        }

        inline SizeT GetCount(const KeyType& key) const
        {
            return Impl.count(key);
        }

        inline bool KeyExists(const KeyType& key) const
        {
            return Impl.contains(key);
        }

        inline auto Find(const KeyType& key)
        {
            return Impl.find(key);
        }

        inline auto Find(const KeyType& key) const
        {
            return Impl.find(key);
        }

        inline auto Begin()
        {
            return Impl.begin();
        }

        inline auto End()
        {
            return Impl.end();
        }

        inline auto Begin() const
        {
            return Impl.begin();
        }

        inline auto End() const
        {
            return Impl.end();
        }

        inline ValueType& operator[](const KeyType& key)
        {
            return Impl[key];
        }

        inline ValueType& Get(const KeyType& key)
        {
            return Impl.at(key);
        }

        inline const ValueType& Get(const KeyType& key) const
        {
            return Impl.at(key);
        }

        inline bool TryGet(const KeyType& key, ValueType& outValue) const
        {
            auto it = Find(key);
            if (it != End())
            {
                outValue = it->second;
            }
            return it != End();
        }

        inline void Add(const KeyType& key, const ValueType& value)
        {
            Impl.insert({ key, value });
        }

        inline void Add(const TPair<KeyType, ValueType>& pair)
        {
            Impl.insert({ pair.first, pair.second });
        }

        inline void AddRange(const THashMap<KeyType, ValueType>& from)
        {
            for (const auto& [key, value] : from)
            {
                Add({ key, value });
            }
        }

        template<typename... Args>
        auto Emplace(Args... args)
        {
            return Impl.emplace(args...);
        }

        void Remove(const KeyType& key)
        {
            Impl.erase(key);
        }

        void Remove(ankerl::unordered_dense::map<KeyType, ValueType, HashFunc<KeyType>>::iterator iterator)
        {
            Impl.erase(iterator);
        }

        void Clear()
        {
            Impl.clear();
        }

        inline auto begin() { return Impl.begin(); }
        inline auto end() { return Impl.end(); }

        inline const auto begin() const { return Impl.begin(); }
        inline const auto end() const { return Impl.end(); }

    private:
        ankerl::unordered_dense::map<KeyType, ValueType, HashFunc<KeyType>> Impl;
    };

    template<typename T>
    class THashSet
    {
    public:
        THashSet() : impl({})
        {
        }

        THashSet(std::initializer_list<T> elements) : impl(elements)
        {
        }

        inline void Add(const T& item)
        {
            impl.insert(item);
        }

        inline void Remove(const T& item)
        {
            impl.erase(item);
        }

        inline bool Empty() const
        {
            return impl.empty();
        }

        inline bool Contains(const T& item) const
        {
            return impl.contains(item);
        }

        inline const T& Find(const T& item) const
        {
            return *impl.find(item);
        }

        bool Exists(std::function<bool(const T&)> pred) const
        {
            for (auto it = impl.begin(); it != impl.end(); ++it)
            {
                if (pred(*it))
                {
                    return true;
                }
            }

            return false;
        }

        inline void Clear()
        {
            impl.clear();
        }

        inline SizeT GetSize() const { return impl.size(); }

        auto Begin() { return impl.begin(); }
        auto End() { return impl.end(); }

        auto begin() { return impl.begin(); }
        auto end() { return impl.end(); }

        auto begin() const { return impl.begin(); }
        auto end() const { return impl.end(); }

        auto cbegin() const { return impl.cbegin(); }
        auto cend() const { return impl.cend(); }

    private:
        ankerl::unordered_dense::set<T, HashFunc<T>> impl{};
    };
    
} // namespace Fusion
