#ifndef BINARY_LOVE_3_H_
#define BINARY_LOVE_3_H_
#pragma once

#include <type_traits>
#include <vector>
#include <cstdint>
#include <optional>
#include <concepts>
#include <string>
#include <iterator>

#pragma region DECLARATIONS
namespace BinaryLove3
{
    namespace details
    {
        struct any_type;

        template<class T, class... Args>
        struct member_counter;

        template<class T, class Functor, bool Const, class... Args>
        void iterate_over_elements(std::conditional_t<Const, const T&, T&> t_, Args&&... args_);

        template<class T>
        concept iterable =
            requires (T v_)
        {
            typename T::iterator;
            typename T::value_type;
            { std::begin(v_) } -> std::same_as<typename T::iterator>;
            { std::end(v_) } -> std::same_as<typename T::iterator>;
        };

        template<class T>
        concept const_iterable =
            requires (T v_)
        {
            typename T::const_iterator;
            typename T::value_type;
            { std::cbegin(v_) } -> std::same_as<typename T::const_iterator>;
            { std::cend(v_) } -> std::same_as<typename T::const_iterator>;
        };

        template<class T>
        concept compatible_iterable =
            (iterable<T> || const_iterable<T>) && requires (T v_)
        {
            {std::inserter<T>(v_, std::end(v_))} -> std::same_as<std::insert_iterator<T>>;
        };

        template<class T>
        concept random_access_iterable =
            iterable<T> &&
            std::random_access_iterator<typename T::iterator> &&
            std::is_standard_layout_v<typename T::value_type> &&
            std::is_trivial_v<typename T::value_type> &&
            requires (T v_)
        {
            T(10, typename T::value_type());
        };

        struct serialize_functor;
        struct deserialize_functor;
    }

    template<class T>
    std::vector<std::byte> serialize(const T& var_)
        requires(std::is_standard_layout_v<T> && std::is_trivial_v<T>);

    template<class T>
    std::vector<std::byte> serialize(const T& var_)
        requires(details::template random_access_iterable<T>);

    template<class T>
    std::vector<std::byte> serialize(const T& var_)
        requires(details::template compatible_iterable<T> && 
            !details::template random_access_iterable<T>);

    template<class T>
    std::vector<std::byte> serialize(const T& var_) 
        requires(std::is_aggregate_v<T>
        && !(std::is_standard_layout_v<T> && std::is_trivial_v<T>));

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(std::is_standard_layout_v<T> && std::is_trivial_v<T>);

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(details::template random_access_iterable<T>);

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(details::template compatible_iterable<T> &&
            !details::template random_access_iterable<T>);

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(std::is_aggregate_v<T>
            && !(std::is_standard_layout_v<T>&& std::is_trivial_v<T>));

    template<class T>
    bool deserialize(const std::vector<std::byte>& data_, T& obj_);
}
#pragma endregion

#pragma region DEFINITIONS_
namespace BinaryLove3
{
    template<class T>
    std::vector<std::byte> serialize(const T& var_)
        requires(std::is_standard_layout_v<T> && std::is_trivial_v<T>)
    {
        std::vector<std::byte> data(sizeof(T));
        std::memcpy(data.data(), &var_, sizeof(T));
        return data;
    }

    template<class T>
    std::vector<std::byte> serialize(const T& var_)
        requires(details::random_access_iterable<T>)
    {
        using value = typename T::value_type;
        size_t size = std::size(var_);
        std::vector<std::byte> data(sizeof(uint64_t) + sizeof(value) * size);

        uint64_t size64 = static_cast<uint64_t>(size) * sizeof(value) + sizeof(uint64_t);
        std::memcpy(data.data(), &size64, sizeof(uint64_t));

    	if(size != 0)
    	{
			const value* beg = &(*std::begin(var_));
			std::memcpy(data.data() + sizeof(uint64_t), beg, static_cast<size_t>(size64 - sizeof(uint64_t)));
    	}
    	
        return data;
    }

    template<class T>
    std::vector<std::byte> serialize(const T& var_)
        requires(details::template compatible_iterable<T> && !details::template random_access_iterable<T>)
    {
        std::vector<std::byte> data(sizeof(uint64_t));
        uint64_t size64 = sizeof(uint64_t);

        std::vector<std::byte> element_data;
        for (const typename T::value_type& e : var_)
        {
            element_data = serialize(e);
            size64 += static_cast<uint64_t>(element_data.size());
            data.insert(std::end(data), std::begin(element_data), std::end(element_data));
        }

        std::memcpy(data.data(), &size64, sizeof(uint64_t));
        return data;
    }

    template<class T>
    std::vector<std::byte> serialize(const T& var_) requires(std::is_aggregate_v<T>
        && !(std::is_standard_layout_v<T> && std::is_trivial_v<T>))
    {
        std::vector<std::byte> data;
        details::template iterate_over_elements<T, details::serialize_functor,
            true,
            std::vector<std::byte>&>
            (var_, data);
        return data;
    }

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(std::is_standard_layout_v<T> && std::is_trivial_v<T>)
    {
        if (cur_ + sizeof(T) > end_)
            return false;

        T ret;
        std::memcpy(&ret, cur_, sizeof(T));
        cur_ += sizeof(T);
        obj_ = ret;
        return true;
    }

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(details::template random_access_iterable<T>)
    {
        using value = typename T::value_type;

        if (cur_ + sizeof(uint64_t) > end_)
            return false;
        
        uint64_t size{ 0llu };
        std::memcpy(&size, cur_, sizeof(uint64_t));
        
        if (cur_ + size > end_)
            return false;
        
        cur_ += sizeof(uint64_t);
        size -= sizeof(uint64_t);
        
        T elems(static_cast<size_t>(size) / sizeof(value), value());
        std::memcpy(elems.data(), cur_, static_cast<size_t>(size));
        cur_ += size;
        obj_ = std::move(elems);
        return true;
    }

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(details::template compatible_iterable<T> &&
            !details::template random_access_iterable<T>)
    {
        using value = typename T::value_type;

        if (cur_ + sizeof(uint64_t) > end_)
            return false;
        
        uint64_t size{ 0llu };
        std::memcpy(&size, cur_, sizeof(uint64_t));
        
        const std::byte* end = cur_ + size;
        cur_ += sizeof(uint64_t);
        if (end > end_)
            return false;
        
        T ret;
        auto inserter = std::inserter(ret, std::end(ret));
        while (cur_ < end)
        {
            value temp;
            if (!deserialize(cur_, end, temp)) return false;
            inserter = std::move(temp);
        }
        
        obj_ = std::move(ret);
        
        return true;
    }

    template<class T>
    bool deserialize(const std::byte*& cur_, const std::byte* end_, T& obj_)
        requires(std::is_aggregate_v<T>
            && !(std::is_standard_layout_v<T>&& std::is_trivial_v<T>))
    {
        bool error = false;
        T ret;
        details::iterate_over_elements<T, details::deserialize_functor, false, const std::byte*&, const std::byte*, bool&>
            (ret, cur_, std::move(end_), error);

        if (error) return false;
        
        obj_ = std::move(ret);
        return true;
    }

    template<class T>
    bool deserialize(const std::vector<std::byte>& data_, T& obj_)
    {
        T v;
        const std::byte* beg = data_.data();
        const std::byte* end = data_.data() + data_.size();
        bool result = deserialize(beg, end, v);
        if(result)
            obj_ = std::move(v);
        return result;
    }

	namespace details
	{
        struct any_type
        {
            template<class T>
            operator T() {};
        };

        template<class T, class... Args>
        struct member_counter
        {
            constexpr static size_t f(int32_t*)
            {
                return sizeof...(Args) - 1;
            }

            template<class U = T, class enabld = decltype(U{ Args{}... }) >
            constexpr static size_t f(std::nullptr_t)
            {
                return member_counter<T, Args..., any_type>::value;
            }

            constexpr static auto value = f(nullptr);
        };

        template<class T, class Functor, bool Const, class... Args>
        void iterate_over_elements(std::conditional_t<Const, const T&, T&> t_, Args&&... args_)
        {
#define DEFINE_FIELD(IDX)\
            if constexpr (std::is_class_v<decltype(__v##IDX)> && std::is_aggregate_v<decltype(__v##IDX)>)\
                iterate_over_elements<decltype(__v##IDX), Functor, Const, Args...>(__v##IDX, std::forward<Args>(args_)...);\
            else Functor::template executor<decltype(__v##IDX)>(__v##IDX, std::forward<Args>(args_)...);\

            if constexpr (member_counter<T>::value == 1)
            {
                auto& [__v0] = t_;
                DEFINE_FIELD(0)
            }
            if constexpr (member_counter<T>::value == 2)
            {
                auto& [__v0, __v1] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
            }
            if constexpr (member_counter<T>::value == 3)
            {
                auto& [__v0, __v1, __v2] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
            }
            if constexpr (member_counter<T>::value == 4)
            {
                auto& [__v0, __v1, __v2, __v3] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
                DEFINE_FIELD(3);
            }

            if constexpr (member_counter<T>::value == 5)
            {
                auto& [__v0, __v1, __v2, __v3, __v4] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
                DEFINE_FIELD(3);
                DEFINE_FIELD(4);
            }

            if constexpr (member_counter<T>::value == 6)
            {
                auto& [__v0, __v1, __v2, __v3, __v4, __v5] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
                DEFINE_FIELD(3);
                DEFINE_FIELD(4);
                DEFINE_FIELD(5);
            }

            if constexpr (member_counter<T>::value == 7)
            {
                auto& [__v0, __v1, __v2, __v3, __v4, __v5, __v6] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
                DEFINE_FIELD(3);
                DEFINE_FIELD(4);
                DEFINE_FIELD(5);
                DEFINE_FIELD(6);
            }

            if constexpr (member_counter<T>::value == 8)
            {
                auto& [__v0, __v1, __v2, __v3, __v4, __v5, __v6, __v7] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
                DEFINE_FIELD(3);
                DEFINE_FIELD(4);
                DEFINE_FIELD(5);
                DEFINE_FIELD(6);
                DEFINE_FIELD(7);
            }

            if constexpr (member_counter<T>::value == 9)
            {
                auto& [__v0, __v1, __v2, __v3, __v4, __v5, __v6, __v7, __v8] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
                DEFINE_FIELD(3);
                DEFINE_FIELD(4);
                DEFINE_FIELD(5);
                DEFINE_FIELD(6);
                DEFINE_FIELD(7);
                DEFINE_FIELD(8);
            }

            if constexpr (member_counter<T>::value == 10)
            {
                auto& [__v0, __v1, __v2, __v3, __v4, __v5, __v6, __v7, __v8, __v9] = t_;
                DEFINE_FIELD(0);
                DEFINE_FIELD(1);
                DEFINE_FIELD(2);
                DEFINE_FIELD(3);
                DEFINE_FIELD(4);
                DEFINE_FIELD(5);
                DEFINE_FIELD(6);
                DEFINE_FIELD(7);
                DEFINE_FIELD(8);
                DEFINE_FIELD(9);
            }
#undef DEFINE_FIELD
        };

        
        struct serialize_functor
        {
            template<class T>
            static inline void executor(const T& obj_, std::vector<std::byte>& data_)
            {
                std::vector<std::byte> data = serialize(obj_);
                data_.insert(std::end(data_), std::begin(data), std::end(data));
            }
        };

        struct deserialize_functor
        {
            template<class T>
            static inline void executor(T& obj_, const std::byte*& cur_, const std::byte* end_, bool& error_)
            {
                if (error_) return;
                T temp;
                error_ = !deserialize(cur_, end_, temp);
                if (error_)
                {
                    return;
                }

                obj_ = std::move(temp);
            }
        };
    };
}
#pragma endregion

#endif
