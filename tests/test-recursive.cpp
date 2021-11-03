#include <ahsohtoa/recursive.hpp>

struct vec3
{
  float x, y, z;
};
struct color
{
  float r, g, b, a;
};
struct physics_component
{
  vec3 position;
  vec3 speed;
  vec3 acceleration;
};

struct render_component
{
  color col;
};

struct entity
{
  physics_component physics;
  render_component render;
};

int main()
{
  ahso::recursive_arrays<std::vector, entity> e;

  e.reserve(500);
  e.resize(30);

  e.components<float>(access(entity, physics.speed.y));

  e.get<float>(access(entity, physics.speed.y), 123) = 1.0;

  auto x = e.components<float>(access(entity, physics.position.x));
  auto y = e.components<float>(access(entity, physics.position.y));
  auto z = e.components<float>(access(entity, physics.position.z));

  auto dx = e.components<float>(access(entity, physics.position.x));
  auto dy = e.components<float>(access(entity, physics.position.y));
  auto dz = e.components<float>(access(entity, physics.position.z));

  for(int i = 0; i < e.size(); i++) {
      x[i] += dx[i];
      y[i] += dy[i];
      z[i] += dz[i];
  }
}

/*
void tests()
{

  ahso::recursive_arrays<std::vector, entity> e;

  e.reserve(500);
  e.resize(30);

#define access(root, member) [ ] { constexpr root e{}; return (ahso::member_offset)(((intptr_t) &e.member) - intptr_t(&e)); }()
  int offset = access(entity, physics.speed.y);
  printf("%d ?? \n", offset);
  assert(offset == 3 * 4 + 4);
//#define get_member(entity, physics, speed, y) [] { }

  int res = ac.get_index<entity>(yt);
  assert(res == 1);

  std::span<float> speed_y = e.components<float>(access(entity, physics.speed.y));
  assert(speed_y.size() == 30);



  // Ideal:
  constexpr ahso::ftuple<&entity::physics, &decltype(entity::physics)::speed, &decltype(decltype(entity::physics)::speed)::x> yt;
  static constexpr ahso::access_recursive<entity> ac;
  for(int i = 0; i < sizeof(entity); i++)
  {
      printf("%d %d\n", i, ac.index_map[i]);
  }

}

using namespace ahso;

  struct bar {
    int x, y;
  };
struct foo {
  bar b;
  int a;
};


static_assert(std::is_same_v<deaggregate<bar>::type, std::tuple<int, int>>);
static_assert(std::is_same_v<deaggregate<foo>::type, std::tuple<std::tuple<int, int>, int>>);

static_assert(std::is_same_v<
    boost::mp11::mp_flatten<std::tuple<int, std::tuple<int, int>, std::tuple<int, int>>>
  , std::tuple<int, int, int, int, int>
  >);



struct foo2 {
  bar b;
  int a;
  struct {

    int e;
    bar c;
    bar d;
    struct {
      bar aa;
    } z;
  } c;
};
static_assert(std::is_same_v<deaggregate_t<foo2>, std::tuple<
  int, int, // b
  int, // a
  int, // c.e
  int, int, // c.c
  int, int, // c.d
  int, int // c.z.aa
  >>);
*/