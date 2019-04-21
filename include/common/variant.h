#include <iostream>
#include <typeinfo>

/* Inspire by https://www.ojdip.net/2013/10/implementing-a-variant-type-in-cpp/ */
template <class... Args>
struct variant_static_stats;

template <class T>
struct variant_static_stats<T> {
    static const size_t size = sizeof(T);
    static const size_t align = alignof(T);
};

template <class T, class U, class... Vs>
struct variant_static_stats<T, U, Vs...> {
    static const size_t size =
        sizeof(T) > sizeof(U) ?
        variant_static_stats<T, Vs...>::size :
        variant_static_stats<U, Vs...>::size;
    static const size_t align =
        alignof(T) > alignof(U) ?
        variant_static_stats<T, Vs...>::align :
        variant_static_stats<U, Vs...>::align;
};

template <class... Args>
struct variant_helper;

template <typename F, typename... Ts>
struct variant_helper<F, Ts...> {
    inline static void destroy(size_t id, void *data) {
        if (id == typeid(F).hash_code()) {
            reinterpret_cast<F*>(data)->~F();
        } else {
            variant_helper<Ts...>::destroy(id, data);
        }
    }
    inline static void move(size_t id, void *source, void* dest) {
        if (id == typeid(F).hash_code()) {
            new (dest) F(std::move(*reinterpret_cast<F*>(source)));
        } else {
            variant_helper<Ts...>::move(id, source, dest);
        }
    }
    inline static void copy(size_t id, const void *source, void* dest) {
        if (id == typeid(F).hash_code()) {
            new (dest) F(*reinterpret_cast<const F*>(source));
        } else {
            variant_helper<Ts...>::copy(id, source, dest);
        }
    }
};

template <>
struct variant_helper<>{
    inline static void destroy(size_t, void *) {}
    inline static void move(size_t, void *, void*) {}
    inline static void copy(size_t, const void *, void*) {}
};

template <typename... Ts>
class Variant {
public:
    Variant() : m_current_type(invalid_type()) {}

    Variant(const Variant<Ts...>& old) : m_current_type(old.m_current_type) {
        variant_helper<Ts...>::copy(old.m_current_type, &old.m_data, &m_data);
    }
    Variant(const Variant<Ts...>&& old) : m_current_type(old.m_current_type) {
        variant_helper<Ts...>::move(old.m_current_type, &old.m_data, &m_data);
    }
    Variant<Ts...>& operator= (Variant<Ts...> old) {
        std::swap(m_current_type, old.m_current_type);
        std::swap(m_data, old.m_data);
        return *this;
    }
    ~Variant() {
        variant_helper<Ts...>::destroy(m_current_type, &m_data);
    }

    template <typename T>
    T& get() {
        if (m_current_type == typeid(T).hash_code()) {
            return *reinterpret_cast<T*>(&m_data);
        } else {
            throw std::bad_cast();
        }
    }
    template <typename T>
    const T& get() const {
        if (m_current_type == typeid(T).hash_code()) {
            return *reinterpret_cast<const T*>(&m_data);
        } else {
            throw std::bad_cast();
        }
    }
    template <typename T, typename... Args>
    void set(Args&&... args) {
        variant_helper<Ts...>::destroy(m_current_type, &m_data);
        new (&m_data) T(std::forward<Args>(args)...);
        m_current_type = typeid(T).hash_code();
    }

private:
    static const size_t m_size = variant_static_stats<Ts...>::size;
    static const size_t m_align = variant_static_stats<Ts...>::align;

    static inline size_t invalid_type() {
        return typeid(void).hash_code();
    }
    size_t m_current_type;
    typename std::aligned_storage<m_size, m_align>::type m_data;
};
