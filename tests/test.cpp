#include <ahsohtoa/ahsohtoa.hpp>

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

    // Not mandatory but it allows a better syntax when using get(index);
    // Ideally metaclasses would be able to synthesize that from the original type.
    struct reference_type {
      physics_component& physics;
      render_component& render;
    };
};

int main()
{
    ahso::arrays<std::vector, entity> e;

    e.reserve(500);

    e.add_entity({
        .physics {
            .position { 0., 0., 0. }
          , .acceleration { 1., 0., 0. }
        },
        .render {
            .col { 0., 1., 0., 1. }
        }
    });

    e.get<physics_component>(0).position.x = 2;

    // Print all the colors before the change
    for(auto& [col] : e.components<render_component>()) {
        printf("%f %f %f %f\n", col.r, col.g, col.b, col.a);
    }

    e[0].render.col.r = 1.0;

    // Print all the colors after the change
    for(auto& [col] : e.components<render_component>()) {
        printf("%f %f %f %f\n", col.r, col.g, col.b, col.a);
    }

    assert(e.size() == 1);
    e.erase(0);
    assert(e.size() == 0);

    e.resize(100);
}