#pragma once

#include <boost/mp11.hpp>
#include <boost/pfr.hpp>

#include <span>

#include <type_traits>

namespace ahso
{
template <typename T>
concept aggregate = std::is_aggregate_v<T>;

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

template <auto... funcs>
struct ftuple
{
};

template <typename T>
struct to_type
{
  using type = std::decay_t<T>;
};

template <typename Obj>
struct access_recursive
{
  std::array<int, sizeof(Obj) * 2> index_map{};

  consteval access_recursive()
  {
    constexpr Obj e{};
    int cur_offset = 0, cur_index = 0;
    compute_offsets<Obj>(cur_offset, cur_index);
  }

  template <typename T>
  constexpr int compute_offsets(int& cur_offset, int cur_index = 0)
  {
    int orig = cur_offset;

    auto compute_rec = [&]<typename Field>(to_type<Field> field)
    {
      index_map[cur_offset] = cur_index;

      cur_index += 1;
      cur_offset += sizeof(Field); // note that it requires packed structs

      if constexpr (ahso::aggregate<Field>)
      {
        // First member will have the same address, we go backwards
        cur_offset -= sizeof(Field);
        cur_index -= 1;
        cur_index = compute_offsets<Field>(cur_offset, cur_index);
      }
    };

    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (compute_rec(to_type<std::decay_t<decltype(boost::pfr::get<N>(T{}))>>{}),
       ...);
    }
    (std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
    return cur_index;
  }
};

enum class member_offset : int
{
};

template <template <typename...> typename Container, typename T>
struct recursive_arrays
{
  using tuple_type = deaggregate_t<T>;
  using vec_type = boost::mp11::mp_transform<Container, tuple_type>;
  using indices = std::make_index_sequence<std::tuple_size_v<tuple_type>>;
  vec_type vec;
  static constexpr access_recursive<T> accessors{};

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
  /* TODO
  std::size_t add_entity(T e)
  {
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      (std::get<N>(vec).push_back(std::move(boost::pfr::get<N>(e))), ...);
    }
    (indices{});
    return size() - 1;
  }
  */

  //! Get a view on the list of components
  template <typename Component>
  std::span<Component> components(member_offset offset)
  {
    std::span<Component> ret;
    [&]<std::size_t... N>(std::index_sequence<N...>)
    {
      // Go from offset of member to index in flat tuple
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
#define access(root, member) ((ahso::member_offset)offsetof(root, member))
