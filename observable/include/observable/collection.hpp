#pragma once
#include <unordered_set>

#include <observable/value.hpp>

#include <observable/detail/compiler_config.hpp>
OBSERVABLE_BEGIN_CONFIGURE_WARNINGS


namespace observable {

template <
    typename ValueType, 
    template<typename ValueType, typename... ContainerArgs> typename ContainerType = std::unordered_set,
    typename... ContainerArgs
>
class collection : public value<ContainerType<ValueType, ContainerArgs...>>
{
    using change_subject = subject<void(ValueType const &, bool)>;
    using base_class = value<ContainerType<ValueType, ContainerArgs...>>;

public:
    using container_type = ContainerType<ValueType, ContainerArgs...>;

    //! Create a default-constructed observable collection.
    //!
    //! The backing container will be default constructed and empty
    constexpr collection() =default;

    //! Create an initialized observable collection.
    //!
    //! \param initial_value The observable collection's initial value.
    constexpr collection(std::initializer_list<ValueType> initial_value) :
        base_class(container_type(initial_value))
    {}

    template <typename Callable>
    auto subscribe_changes(Callable && observer) const
    {
        static_assert(detail::is_compatible_with_subject<Callable, change_subject>::value,
            "Observer is not valid. Please provide an observer that takes ValueType as"
            "its first argument and a boolean as its second argument");

        return subscribe_changes_impl(std::forward<Callable>(observer));
    }

    //! Insert a new value into collection, possibly notifying any subscribed observers.
    //!
    //! The new value is inserted respecting the rules of the underlying container, if
    //! it is not possible to add no observers will be notified
    //!
    //! \param new_value The new value to add.
    //! \throw readonly_value if the value has an associated updater.
    //! \see subject<void(Args ...)>::notify()
    bool insert(ValueType new_value)
    {
        return insert_impl(std::move(new_value));
    }

    //! Emplace a new value into collection, possibly notifying any subscribed observers.
    //!
    //! The new value is emplaced respecting the rules of the underlying container, if
    //! it is not possible to add no observers will be notified
    //!
    //! \param new_value The new value to add.
    //! \throw readonly_value if the value has an associated updater.
    //! \see subject<void(Args ...)>::notify()
    bool emplace(ValueType && new_value)
    {
        return emplace_impl(std::forward(new_value));
    }

    //! Remove a value from collection, possibly notifying any subscribed observers.
    //!
    //! The new value is removed respecting the rules of the underlying container, if it
    //! is not found in the container no observers will be notified
    //!
    //! \param new_value The new value to add.
    //! \throw readonly_value if the value has an associated updater.
    //! \see subject<void(Args ...)>::notify()
    bool remove(ValueType value)
    {
        return remove_impl(std::move(value));
    }

private:
    template <typename Callable>
    auto subscribe_changes_impl(Callable && observer) const
    {
        return change_observers_.subscribe(std::forward<Callable>(observer));
    }

    bool insert_impl(ValueType new_value)
    {
        const auto [it, inserted] = value_.insert(new_value);

        if (inserted)
        {
            change_observers_.notify(*it, true);
            void_observers_.notify();
            value_observers_.notify(value_);
        }

        return inserted;
    }

    bool emplace_impl(ValueType new_value)
    {
        const auto [it, inserted] = value_.emplace(new_value);

        if (inserted)
        {
            change_observers_.notify(*it, true);
            void_observers_.notify();
            value_observers_.notify(value_);
        }

        return inserted;
    }

#if __cplusplus > 201703L
    bool remove_impl(ValueType value)
    {
        auto nh = value_.extract(value);
        bool removed = false;

        if (nh)
        {
            removed = true;
            change_observers_.notify(nh.value(), false);
            void_observers_.notify();
            value_observers_.notify(value_);
        }

        return removed;
    }
#else
    bool remove_impl(ValueType value)
    {
        // Without the C++17 extract function, we have to call the
        // change_observer with the reference to the item in the collection
        // before we actually remove it
        bool removed = false;
        
        auto found_it = value_.find(value);

        if (found_it != value_.end())
        {
            change_observers_.notify(*found_it, false);

            value_.erase(found_it);
            void_observers_.notify();
            value_observers_.notify(value_);
            
            removed = true;
        }

        return removed;
    }
#endif

    mutable change_subject change_observers_;
};

}

OBSERVABLE_END_CONFIGURE_WARNINGS