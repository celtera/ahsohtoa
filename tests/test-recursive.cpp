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

struct vec3_ref
{
  float& x, &y, &z;
};
struct color_ref
{
  float& r, &g, &b, &a;
};
struct physics_component_ref
{
  vec3_ref position;
  vec3_ref speed;
  vec3_ref acceleration;
};

struct render_component_ref
{
  color_ref col;
};

struct entity
{
  physics_component physics;
  render_component render;

  // Again, not necessary but makes iteration much more fun
  struct reference_type {
    reference_type(auto t)
        : physics {
        { get<0>(t), get<1>(t), get<2>(t) },
        { get<3>(t), get<4>(t), get<5>(t) },
        { get<6>(t), get<7>(t), get<8>(t) }
        }
        , render {
        { get<9>(t), get<10>(t), get<11>(t), get<12>(t) }
        } {

        }
    physics_component_ref physics;
    render_component_ref render;
  };

};

int main()
{
  ahso::recursive_arrays<std::vector, entity> e;

entity zzzx ;

  e.add_entity({
    .physics {
      { 1000 + 0, 1000 + 1, 1000 + 2 },
      { 1000 + 3, 1000 + 4, 1000 + 5 },
      { 1000 + 6, 1000 + 7, 1000 + 8 }
    }
  , .render {
      { 1000 + 9, 1000 + 10, 1000 + 11, 1000 + 12 }
    }
  });

  assert(e[0].physics.speed.y == 1004);
  assert(e[0].render.col.g == 1010);

  e.reserve(500);
  e.resize(30);

  e.components<float>(access(entity, physics.speed.y));

  e.get<float>(access(entity, physics.speed.y), 123) = 1.0;

  // std::spans can be accessed directly
  auto x = e.components<float>(access(entity, physics.position.x));
  auto y = e.components<float>(access(entity, physics.position.y));
  auto z = e.components<float>(access(entity, physics.position.z));

  auto sx = e.components<float>(access(entity, physics.speed.x));
  auto sy = e.components<float>(access(entity, physics.speed.y));
  auto sz = e.components<float>(access(entity, physics.speed.z));

  // Initialize
  for(int i = 0; i < e.size(); i++) {
      x[i] = 0.f; sx[i] = 1.f;
      y[i] = 0.f; sy[i] = 1.f;
      z[i] = 0.f; sz[i] = 1.f;

      assert(x[i] == 0.f);
  }

  // Move things through span access
  for(int i = 0; i < e.size(); i++) {
      x[i] += sx[i];
      y[i] += sy[i];
      z[i] += sz[i];

      assert(x[i] == 1.f);
  }

  // If one provides a reference_type with a good constructor,
  // this will also work:
  for(int i = 0; i < e.size(); i++) {
      entity::reference_type elt = e[i];
      elt.physics.position.x += elt.physics.speed.x;
      elt.physics.position.y += elt.physics.speed.y;
      elt.physics.position.z += elt.physics.speed.z;

      assert(elt.physics.position.x == 2.f);
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