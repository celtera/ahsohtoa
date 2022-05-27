#pragma once

#include <boost/mp11.hpp>
#include <boost/pfr.hpp>

#include <span>
#include <type_traits>

/**
 * \file ahsohtoa.hpp
 *
 * In this file, we'll assume for the sake of documentation that T is:
 *
 * struct Foo {
 *   int x;
 *   float y;
 * };
 */

namespace ahso
{

/**
 * Yields std::tuple<int, float>
 */
template <typename T>
using tuple_type_t = decltype(boost::pfr::structure_to_tuple(T{}));

/**
 * Transforms a structure into the equivalent structure-of-arrays:
 *
 * T -> std::tuple<std::vector<int>, std::vector<float>>
 */
template <template <typename...> typename Container, typename T>
struct arrays
{
  using tuple_type = tuple_type_t<T>;

  //! Yields std::tuple<std::vector<int>, std::vector<float>>
  using vec_type = boost::mp11::mp_transform<Container, tuple_type>;

  //! Yields std::index_sequence<0, 1>
  using indices = std::make_index_sequence<boost::pfr::tuple_size_v<T>>;

  //! Our data storage
  vec_type vec;

  //! Get the number of stored entities
  std::size_t size() const noexcept { return std::get<0>(vec).size(); }

  //! Reserve storage for the entities (if Container supports it)
  void reserve(std::size_t space)
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).reserve(space), ...);
    }
    (indices{});

    // In expanded & cleaned-up form, the above is simply a compile-time for-loop
    // over all the elements of our tuple:
    //
    // auto do_reserve = [&] (std::index_sequence<0, 1>)
    // {
    //   std::get<0>(vec).reserve(space);
    //   std::get<1>(vec).reserve(space);
    // };
    //
    // do_reserve(std::index_sequence<0, 1>{});
  }

  //! Resize storage for the entities (if Container supports it)
  void resize(std::size_t space)
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).resize(space), ...);
    }
    (indices{});
  }

  //! Create a new entity and return its index
  std::size_t create()
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).push_back({}), ...);
    }
    (indices{});

    return size() - 1;
  }

  //! Removes the entity at the given index
  void erase(std::size_t index)
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).erase(std::get<N>(vec).begin() + index), ...);
    }
    (indices{});
  }

  //! Add a new entity at the end of the storage and return its index.
  std::size_t add_entity(T e)
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).push_back(std::move(boost::pfr::get<N>(e))), ...);
    }
    (indices{});
    return size() - 1;

    // This one is a bit more involved:
    // It destructures T into all its constituents, and push_back those into each respective container.
    // e.g.
    //
    // auto do_push_back = [&] (std::index_sequence<0, 1>)
    // {
    //   float& element_0 = boost::pfr::get<0>(e);
    //   std::get<0>(vec).push_back(element_0);
    //
    //   int& element_1 = boost::pfr::get<1>(e);
    //   std::get<1>(vec).push_back(element_1);
    // };
    //
    // do_push_back(std::index_sequence<0, 1>{});
  }

  //! Get a view on the list of components
  template <typename Component>
  std::span<Component> components()
  {
    return std::get<Container<Component>>(vec);
  }

  //! Get a specific component from an entity
  template <typename Component>
  auto& get(std::size_t index)
  {
    return std::get<Container<Component>>(vec)[index];
  }

  //! Get a reference to all the components of a given entity
  auto get(std::size_t index)
  {
    // If our struct has a reference_type typedef, we construct it.
    // Note: ideally, metaclasses would allow to generate it.
    if constexpr (requires { sizeof(typename T::reference_type); })
    {
      return [&]<std::size_t... N>(std::index_sequence<N...>)
      {
        using ref_type = typename T::reference_type;
        return ref_type{std::get<N>(vec)[index]...};
      }
      (indices{});
    }
    else
    {
      // Otherwise, we return a std::tuple<Component1&, Component2&, ...>
      return [&]<std::size_t... N>(std::index_sequence<N...>)
      {
        return std::tie(std::get<N>(vec)[index]...);
      }
      (indices{});
    }
  }

  //! Get a reference to all the components of a given entity
  auto operator[](std::size_t index) { return get(index); }
};
}
