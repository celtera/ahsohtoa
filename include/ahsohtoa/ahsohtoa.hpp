#pragma once

#include <boost/pfr.hpp>
#include <boost/mp11.hpp>
#include <type_traits>
#include <span>

namespace ahso
{
template<typename T>
using tuple_type_t = decltype(boost::pfr::structure_to_tuple(T{}));

template<template<typename...> typename Container, typename T>
struct arrays {
  using tuple_type = tuple_type_t<T>;
  using vec_type = boost::mp11::mp_transform<Container, tuple_type>;
  using indices = std::make_index_sequence<boost::pfr::tuple_size_v<T>>;
  vec_type vec;

  //! Get the number of stored entities
  std::size_t size() const noexcept {
    return std::get<0>(vec).size();
  }

  //! Reserve storage for the entities (if Container supports it)
  void reserve(std::size_t space)
  {
    [&] <std::size_t... N> (std::index_sequence<N...>) {
      (std::get<N>(vec).reserve(space), ...);
    }(indices{});
  }

  //! Resize storage for the entities (if Container supports it)
  void resize(std::size_t space)
  {
    [&] <std::size_t... N> (std::index_sequence<N...>) {
      (std::get<N>(vec).resize(space), ...);
    }(indices{});
  }

  //! Create a new entity and return its index
  std::size_t create()
  {
    [&] <std::size_t... N> (std::index_sequence<N...>) {
      (std::get<N>(vec).push_back({}), ...);
    }(indices{});
    return size() - 1;
  }

  void erase(std::size_t index)
  {
    [&] <std::size_t... N> (std::index_sequence<N...>) {
      (std::get<N>(vec).erase(std::get<N>(vec).begin() + index), ...);
    }(indices{});
  }

  //! Add a new entity at the end of the storage.
  std::size_t add_entity(T e)
  {
    [&] <std::size_t... N> (std::index_sequence<N...>) {
      (std::get<N>(vec).push_back(std::move(boost::pfr::get<N>(e))), ...);
    }(indices{});
    return size() - 1;
  }

  //! Get a view on the list of components
  template<typename Component>
  std::span<Component> components()
  {
    return std::get<Container<Component>>(vec);
  }

  //! Get a specific component from an entity
  template<typename Component>
  auto& get(std::size_t index)
  {
    return std::get<Container<Component>>(vec)[index];
  }

  //! Get a reference to all the components of a given entity
  auto get(std::size_t index)
  {
    // If our struct has a reference_type typedef, we construct it.
    // Note: ideally, metaclasses would allow to generate it.
    if constexpr(requires { sizeof(typename T::reference_type); })
    {
      return [&] <std::size_t... N> (std::index_sequence<N...>) {
        using ref_type = typename T::reference_type;
        return ref_type{std::get<N>(vec)[index]...};
      }(indices{});
    }
    else
    {
      // Otherwise, we return a std::tuple<Component1&, Component2&, ...>
      return [&] <std::size_t... N> (std::index_sequence<N...>) {
        return std::tie(std::get<N>(vec)[index]...);
      }(indices{});
    }
  }

  //! Get a reference to all the components of a given entity
  auto operator[](std::size_t index) {
    return get(index);
  }
};
}

