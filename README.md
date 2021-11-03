# ahsohtoa - automatic AoS-to-SoA

![C++](https://img.shields.io/badge/language-c%2B%2B-green) [![Sponsor](https://img.shields.io/github/sponsors/jcelerier)](https://github.com/sponsors/jcelerier) ![License](https://img.shields.io/badge/license-GPLv3-brightgreen)  ![Platforms](https://img.shields.io/badge/platforms-all-blue)

Automatic structure-of-array generation for C++20: this library
takes a structure such as:

```C++
struct Foo {
  int a;
  float b;
  struct Bar {
    float x;
    char y;
  } c;
};
```

and, given:
```C++
ahso::arrays<std::vector, Foo> storage_direct;

ahso::recursive_arrays<std::vector, Foo> storage_recursive;
```

will respectively synthesize types which look like:
```C++
// for storage_direct: 
std::tuple<std::vector<int>, std::vector<float>, std::vector<Bar>>

// for storage_recursive: 
std::tuple<std::vector<int>, std::vector<float>, std::vector<float>, std::vector<char>> 
```

## Dependencies

- Boost.PFR
- C++20

## Simple example

```C++
struct vec3 { float x,y,z; };
struct color { float r,g,b,a; };
struct physics_component {
  vec3 position;
  vec3 speed;
  vec3 acceleration;
};

struct render_component {
  color col;
};

struct entity {
  physics_component physics;
  render_component render;
}

int main()
{
  // Define our storage
  ahso::arrays<std::vector, entity> e;

  // Preallocate some space like you would with std::vector
  e.reserve(1000);

  // Add a new entity
  auto index = e.add_entity({
    .physics {
      .position { 0., 0., 0. }
    , .acceleration { 1., 0., 0. }
    },
    .render {
      .col { 0., 1., 0., 1. }
    }
  });

  // Access a specific component for entity "index"
  e.get<physics_component>(index).position.x = 2;
  
  // Or if the proper type is provided (until we get metaclasses, see example): 
  e[index].physics.position.x = 2;

  // Remove an entity
  e.erase(index);
}
```

## Example with all the features

See https://github.com/celtera/ahsohtoa/blob/main/tests/test.cpp !

# Licensing

The library is licensed under GPLv3 + [commercial](https://celtera.dev/).
