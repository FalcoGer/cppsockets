#pragma once

#include <istream>
#include <ostream>
#include <utility>
template <typename T>
class TypeWrapper
{
  private:
    T m_data;

  public:
    TypeWrapper() : m_data {T {}} {}
    explicit TypeWrapper(const T& data) : m_data {data} {}
    explicit TypeWrapper(T&& data) noexcept : m_data {std::move(data)} {}
    TypeWrapper(const TypeWrapper<T>& other) : m_data {other.m_data} {}
    TypeWrapper(TypeWrapper<T>&& other) noexcept : m_data {std::move(other.m_data)} {}

    template <typename... Args>
    TypeWrapper(Args... args) : m_data(T(std::forward<Args>(args)...))
    {}

    template <typename... Args>
    auto operator= (Args... args) -> TypeWrapper<T>&
    {
        m_data = T(std::forward<Args>(args)...);
        return *this;
    }


    ~TypeWrapper() = default;

    auto operator= (const TypeWrapper<T>& other) -> TypeWrapper<T>&
    {
        m_data = other.m_data;
        return *this;
    }

    auto operator= (TypeWrapper<T>&& other)  noexcept -> TypeWrapper<T>&
    {
        m_data = std::move(other.m_data);
        return *this;
    }

    auto operator= (T&& data) noexcept -> TypeWrapper&
    {
        m_data = std::move(data);
        return *this;
    }
    auto operator= (const T& data) -> TypeWrapper&
    {
        m_data = data;
        return *this;
    }

    explicit operator T () const { return m_data; }

    auto     data() const noexcept -> T { return m_data; }

    auto     operator<< (std::ostream& stream) const -> std::ostream&
    {
        return (stream << m_data);
    }

    auto     operator>> (std::istream& stream) -> std::istream&
    {
        return (stream >> m_data);
    }

};
