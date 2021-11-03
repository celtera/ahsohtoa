#pragma once

#include <boost/mp11.hpp>
#include <boost/pfr.hpp>

#include <span>

#include <type_traits>

/**
 * \file ahsohtoa.hpp
 *
 * In this file, we'll assume for the sake of documentation that T is Foo in the following:
 *
 * struct Bar {
 *   struct Impl {
 *     char c;
 *   } impl;
 * };
 *
 * struct Foo {
 *   int x;
 *   float y;
 *   Bar z;
 * };
 */
namespace ahso
{
//! Check if a type is an aggregate
template <typename T>
concept aggregate = std::is_aggregate_v<T>;

//! Recursively flattens std::tuple<int, float, std::tuple<std::tuple<char>>>
//! into std::tuple<int, float, char>
template <typename T>
struct flatten
{
  using type = T;
};
template <typename T>
using flatten_t = typename flatten<T>::type;

template <typename... R>
struct flatten<std::tuple<R...>>
{
  using type = boost::mp11::mp_flatten<std::tuple<flatten_t<R>...>>;
};

//! Recursively converts an aggregate into a flattened tuple:
//! deaggregate_t<T> == std::tuple<int, float, char>
template <typename T>
struct deaggregate;

template <typename T>
using deaggregate_t = flatten_t<typename deaggregate<T>::type>;

template <typename T>
struct deaggregate
{
  using type = flatten_t<T>;
};

template <aggregate T>
struct deaggregate<T>
{
  using tuple_type
      = boost::mp11::mp_flatten<decltype(boost::pfr::structure_to_tuple(T{}))>;
  using ftype = boost::mp11::mp_transform<deaggregate_t, tuple_type>;
  using type = boost::mp11::mp_transform<flatten_t, ftype>;
};

//! Utility to pass a type as argument
template <typename T>
struct to_type
{
  using type = T;
};

//! Used as a type-safe index to access fields
enum class member_offset : int
{
};

/**
 * Implements a mapping from offsets in a structure, to indices.
 */
template <typename T>
struct access_recursive
{
  // index_map[offsetof(T, x)] = 0;
  // index_map[1] = 0
  // index_map[2] = 0
  // index_map[3] = 0
  // index_map[offsetof(T, y)] = 1;
  // index_map[5] = 0;
  // index_map[6] = 0;
  // index_map[7] = 0;
  // index_map[offsetof(T, z.impl.c)] = 2;
  // index_map[9] = 0;
  // index_map[10] = 0;
  // index_map[11] = 0;
  std::array<int, sizeof(T)> index_map{};

  // We want to make sure that this computation occurs at compile-time
  consteval access_recursive()
  {
    int cur_offset = 0, cur_index = 0;
    compute_offsets<T>(cur_offset, cur_index);
  }

private:
  // Here we just increment by sizeof(Field) on each new field
  template <typename U>
  constexpr int compute_offsets(int& cur_offset, int cur_index = 0)
  {
    // What we use to recurse
    auto compute_rec = [&]<typename Field>(to_type<Field> field)
    {
      index_map[cur_offset] = cur_index;

      // If the field is an aggregate, we recurse
      if constexpr (ahso::aggregate<Field>)
      {
        cur_index = compute_offsets<Field>(cur_offset, cur_index);
      }
      else
      {
        // Otherwise, we increment our indices
        cur_index += 1;
        cur_offset += sizeof(Field); // note that it requires packed structs
      }
    };

    // Call the above on every field
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (compute_rec(to_type<boost::pfr::tuple_element_t<N, U>>{}), ...);
    }
    (std::make_index_sequence<boost::pfr::tuple_size_v<U>>{});
    return cur_index;
  }
};

//! If a type is an aggregate, go as deep as possible until we get a non-aggregate element.
//! T t;
//! access_deepest_aggregate_first_element(t) == t.x;
//! access_deepest_aggregate_first_element(t.z) == t.z.impl.c;
template<typename F>
auto& access_deepest_aggregate_first_element(F& field) {
  if constexpr(aggregate<F>)
    return access_deepest_aggregate_first_element(boost::pfr::get<0>(field));
  else
    return field;
}

//! Utilities to access the nth non-aggregate element of an aggregate, recursively.
//! nth_element_type<0, T> == int;
//! nth_element_type<1, T> == float;
//! nth_element_type<2, T> == char;
template<std::size_t N, typename T>
struct nth_element {
  using type = std::decay_t<decltype(std::get<N>(deaggregate_t<T>{}))>;
};

template<std::size_t N, typename T>
using nth_element_type = typename nth_element<N, T>::type;

namespace detail
{
template<std::size_t N, typename T, typename U>
nth_element_type<N, T>* get_rec_impl(U& field, int& k, nth_element<N, T> )
{
  nth_element_type<N, T>* ptr{};
  if(k == N)
  {
    ptr = &access_deepest_aggregate_first_element(field);
    ++k;
  }
  else
  {
    if constexpr(aggregate<U>)
    {
      [&]<std::size_t... W>(std::index_sequence<W...>)
      {
        ((ptr = get_rec_impl(boost::pfr::get<W>(field), k, nth_element<N, T> {})) || ...);
        //(std::get<W>(vec).reserve(space), ...);
      } (std::make_index_sequence<boost::pfr::tuple_size_v<U>>{});
    }
    else
    {
      ++k;
    }
  }

  return ptr;
}
}

//! Accesses the nth member of an aggregate, recursively.
//! T t;
//! get_rec<0>(t) == t.x;
//! get_rec<1>(t) == t.y;
//! get_rec<2>(t) == t.z.impl.c;
template<std::size_t N, typename T>
nth_element_type<N, T>& get_rec(T& t)
{
  nth_element_type<N, T>* ptr{};
  constexpr access_recursive<T> rec;
  int k = 0;

  boost::pfr::for_each_field(t, [&] <typename F> (F& field) {
      if(ptr)
        return;
      if(k == N)
      {
        ptr = &access_deepest_aggregate_first_element(field);
      }

      if constexpr(aggregate<F>)
      {
        // Go look in it
        if(auto ret = detail::get_rec_impl(field, k, nth_element<N, T>{})) {
          ptr = ret;
        }
      }
      else
      {
        k++;
      }
  });

  return *ptr;
}


/**
 * Transforms a structure into the equivalent structure-of-arrays, recursively:
 *
 * T -> std::tuple<std::vector<int>, std::vector<float>, std::vector<char>>
 */
template <template <typename...> typename Container, typename T>
struct recursive_arrays
{
  //! Yields std::tuple<int, float, char>
  using tuple_type = deaggregate_t<T>;

  //! Yields std::tuple<std::vector<int>, std::vector<float>, std::vector<char>>
  using vec_type = boost::mp11::mp_transform<Container, tuple_type>;

  //! Yields std::index_sequence<0, 1, 2>
  using indices = std::make_index_sequence<std::tuple_size_v<tuple_type>>;

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

  void erase(std::size_t index)
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).erase(std::get<N>(vec).begin() + index), ...);
    }
    (indices{});
  }

  //! Add a new entity at the end of the storage.
  std::size_t add_entity(T e)
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).push_back(std::move(get_rec<N>(e))), ...);
    }
    (indices{});
    return size() - 1;
  }

  //! Get a view on the list of components
  template <typename Component>
  std::span<Component> components(member_offset offset)
  {
    std::span<Component> ret;
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      // Go from offset of member to index in flat tuple
      static constexpr access_recursive<T> accessors{};

      int index = accessors.index_map[(int)offset];

      auto impl = [&]<std::size_t M>()
      {
        if constexpr (requires { ret = std::get<M>(vec); })
          ret = std::get<M>(vec);
      };

      (void)(((N == index) && (impl.template operator()<N>(), true)) || ...);
    }
    (indices{});
    return ret;
  }

  //! Get a specific component from an entity
  template <typename Component>
  auto& get(member_offset offset, std::size_t index)
  {
    return components<Component>(offset)[index];
  }

  //! Get a reference to all the components of a given entity
  auto get(std::size_t index)
  {
    // Nice struct of &: todo
    // Otherwise, we return a std::tuple<Component1&, Component2&, ...>
    auto ret = [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      return std::tie(std::get<N>(vec)[index]...);
    } (indices{});

    if constexpr (requires { sizeof(typename T::reference_type); })
    {
      return typename T::reference_type{ret};
    }
    else
    {
      return ret;
    }
  }

  //! Get a reference to all the components of a given entity
  auto operator[](std::size_t index) { return get(index); }
};

}

//! A macro to simplify calling the functions which take a member
#define access(root, member) ((ahso::member_offset)offsetof(root, member))
