#include <type_traits>
#include <catch/catch.hpp>
#include <observable/collection.hpp>

#include <set>

namespace observable { namespace test {

TEST_CASE("collection/basic collection creation", "[collection]")
{
    SECTION("collections are default-constructible")
    {
        collection<int> { };

        REQUIRE(std::is_default_constructible<collection<int>>::value);
    }

    SECTION("can create initialised value")
    {
        collection<int> { 1, 2, 3, };
    }
}

TEST_CASE("collection/copying", "[collection]")
{
    SECTION("collections are not copy-constructible")
    {
        REQUIRE_FALSE(std::is_copy_constructible<collection<int>>::value);
    }

    SECTION("collections are not copy-assignable")
    {
        REQUIRE_FALSE(std::is_copy_assignable<collection<int>>::value);
    }
}

TEST_CASE("collection/value getter", "[value]")
{
    SECTION("can get value")
    {
        auto col = collection<int>{ 1, 2, 3 };
        REQUIRE(col.get() == std::unordered_set<int>{ 1, 2, 3 });
    }

    SECTION("getter is nothrow")
    {
        auto col = collection<int>{ };
        REQUIRE(noexcept(col.get()));
    }
}

TEST_CASE("collection/conversions", "[value]")
{
    SECTION("can convert to collection_type")
    {
        auto col = collection<int>{ 1, 2, 3 };
        auto c = static_cast<std::unordered_set<int>>(col);
        REQUIRE(c == std::unordered_set<int>{ 1, 2, 3 });

        auto set_col = collection<int, std::set>{ 1, 2, 3 };
        auto set_v = static_cast<std::set<int>>(set_col);
        REQUIRE(set_v == std::set<int>{ 1, 2, 3 });
    }

    SECTION("conversion operator is nothrow")
    {
        auto col = value<int>{ };
        REQUIRE(noexcept(static_cast<int>(col)));
    }
}

TEST_CASE("collection/insertion", "[value]")
{
    SECTION("can insert value")
    {
        auto col = collection<int>{ 1, 2, 3 };
        auto is_inserted = col.insert(4);

        REQUIRE(is_inserted);
        REQUIRE(col.get() == std::unordered_set<int>{ 1, 2, 3, 4 });
    }

    SECTION("can't insert existing value")
    {
        auto col = collection<int>{ 1, 2, 3 };
        auto is_inserted = col.insert(3);

        REQUIRE_FALSE(is_inserted);
        REQUIRE(col.get() == std::unordered_set<int>{ 1, 2, 3 });
    }
}

TEST_CASE("collection/removal", "[value]")
{
    SECTION("can remove existing value")
    {
        auto col = collection<int>{ 1, 2, 3 };
        auto is_removed = col.remove(3);

        REQUIRE(is_removed);
        REQUIRE(col.get() == std::unordered_set<int>{ 1, 2, });
    }

    SECTION("can't remove nonexistent value")
    {
        auto col = collection<int>{ 1, 2, 3 };
        auto is_removed = col.remove(4);

        REQUIRE_FALSE(is_removed);
        REQUIRE(col.get() == std::unordered_set<int>{ 1, 2, 3 });
    }
}

TEST_CASE("collection/subscribing", "[value]")
{
    SECTION("can change collection with no subscribed observers")
    {
        auto col = collection<int>{ 5, 6, 7 };
        col.set({ 3, 4, 5, 6 });

        REQUIRE(col.get() == std::unordered_set<int>{ 3, 4, 5, 6 });
    }

    SECTION("can subscribe to value changes")
    {
        auto call_count = 0;

        auto col = collection<int>{ 1, 2, 3 };
        col.subscribe([&]() { ++call_count; }).release();
        col.set({ 1, 2, 3, 4 });

        REQUIRE(call_count == 1);
    }

    SECTION("can subscribe to value changes on const collections")
    {
        auto call_count = 0;

        auto col = collection<int>{ 1, 2, 3 };
        auto const& const_col = col;
        const_col.subscribe([&]() { ++call_count; });
        col.set({ 1, 2, 3, 4 });

        REQUIRE(call_count == 1);
    }

    SECTION("can subscribe to inserted values")
    {
        auto call_count = 0;
        auto inserted_val = 0;
        auto is_inserted = false;

        auto col = collection<int>{ 1, 2, 3 };
        col.subscribe([&]() { ++call_count; }).release();
        col.subscribe_changes([&](const int& val, bool inserted) { 
            inserted_val = val;
            is_inserted = inserted;
        }).release();

        col.insert(4);

        REQUIRE(call_count == 1);
        REQUIRE(inserted_val == 4);
        REQUIRE(is_inserted);
    }

    SECTION("can subscribe to added values on const collections")
    {
        auto call_count = 0;
        auto inserted_val = 0;
        auto is_inserted = false;

        auto col = collection<int>{ 1, 2, 3 };
        auto const& const_col = col;
        const_col.subscribe([&]() { ++call_count; }).release();
        const_col.subscribe_changes([&](const int& val, bool inserted) {
            inserted_val = val;
            is_inserted = inserted;
        }).release();

        col.insert(4);

        REQUIRE(call_count == 1);
        REQUIRE(inserted_val == 4);
        REQUIRE(is_inserted);
    }

    SECTION("can subscribe to removed values")
    {
        auto call_count = 0;
        auto removed_val = 0;
        auto is_inserted = false;

        auto col = collection<int>{ 1, 2, 3 };
        col.subscribe([&]() { ++call_count; }).release();
        col.subscribe_changes([&](const int& val, bool inserted) {
            removed_val = val;
            is_inserted = inserted;
        }).release();

        col.remove(3);

        REQUIRE(call_count == 1);
        REQUIRE(removed_val == 3);
        REQUIRE_FALSE(is_inserted);
    }

    SECTION("can subscribe to removed values on const collections")
    {
        auto call_count = 0;
        auto removed_val = 0;
        auto is_inserted = false;

        auto col = collection<int>{ 1, 2, 3 };
        auto const& const_col = col;
        const_col.subscribe([&]() { ++call_count; }).release();
        const_col.subscribe_changes([&](const int& val, bool inserted) {
            removed_val = val;
            is_inserted = inserted;
        }).release();

        col.remove(3);

        REQUIRE(call_count == 1);
        REQUIRE(removed_val == 3);
        REQUIRE_FALSE(is_inserted);
    }

    SECTION("setting same value does not trigger subscribers")
    {
        auto call_count = 0;

        auto col = collection<int>{ 1, 2, 3 };
        col.subscribe([&]() { ++call_count; });
        col.subscribe([&](const std::unordered_set<int>&) { ++call_count; });
        col.set({ 1, 2, 3 });

        REQUIRE(call_count == 0);
    }

    SECTION("inserting existing value does not trigger subscribers")
    {
        auto call_count = 0;

        auto col = collection<int>{ 1, 2, 3 };
        col.subscribe([&]() { ++call_count; });
        col.subscribe([&](const std::unordered_set<int>&) { ++call_count; });
        col.insert(3);

        REQUIRE(call_count == 0);
    }

    SECTION("removing non-existing value does not trigger subscribers")
    {
        auto call_count = 0;

        auto col = collection<int>{ 1, 2, 3 };
        col.subscribe([&]() { ++call_count; });
        col.subscribe([&](const std::unordered_set<int>&) { ++call_count; });
        col.remove(4);

        REQUIRE(call_count == 0);
    }

    SECTION("can subscribe and immediately call observer")
    {
        auto col = collection<int>{ 5, 6, 7 };

        auto call_count = 0;
        auto sub = col.subscribe_and_call([&]() { ++call_count; });

        REQUIRE(call_count == 1);
    }

    SECTION("immediately called observer receives the current value")
    {
        auto col = collection<int>{ 5, 6, 7 };

        std::unordered_set<int> call_value{ 3, 4, 5 };
        auto sub = col.subscribe_and_call([&](const std::unordered_set<int>& v) { call_value = v; });

        REQUIRE(call_value == std::unordered_set<int> { 5, 6, 7 });
    }
}

} }