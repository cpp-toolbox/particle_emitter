#ifndef PARTICLE_EMITTER_HPP
#define PARTICLE_EMITTER_HPP

#include <functional>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

#include "sbpt_generated_includes.hpp"

class Particle {
  public:
    Particle(float lifespan_seconds, const glm::vec3 &initial_velocity,
             std::function<glm::vec3(float, float)> velocity_change_func, std::function<float(float)> scaling_func,
             std::function<float(float)> rotation_func, int id);

    void update(float delta_time, glm::mat4 world_to_clip);

    bool operator<(const Particle &that) const;

    bool is_alive() const;

    Transform transform;
    int id;

  private:
    float distance_to_camera;
    float lifespan_seconds;
    float age_seconds;
    glm::vec3 velocity;

    std::function<glm::vec3(float, float)> character_velocity_change_function;
    std::function<float(float)> character_scaling_function;
    std::function<float(float)> character_rotation_degrees_function;
};

class ParticleEmitter {
  public:
    ParticleEmitter(std::function<float()> lifespan_func, std::function<glm::vec3()> initial_velocity_func,
                    std::function<glm::vec3(float, float)> velocity_change_func,
                    std::function<float(float)> scaling_func, std::function<float(float)> rotation_func,
                    std::function<float()> spawn_delay_func, std::function<void(int)> on_particle_spawn_callback,
                    std::function<void(int)> on_particle_death_callback);

    ~ParticleEmitter();

    void update(float delta_time, glm::mat4 world_to_clip);
    std::vector<Particle> get_particles_sorted_by_distance() const;
    Transform transform;

  private:
    void try_to_spawn_new_particle();
    void remove_dead_particles();
    Particle spawn_particle();

    std::vector<Particle> particles;
    std::function<float()> lifespan_func;
    std::function<glm::vec3()> initial_velocity_func;
    std::function<glm::vec3(float, float)> velocity_change_func;
    std::function<float(float)> scaling_func;
    std::function<float(float)> rotation_func;
    std::function<float()> spawn_delay_func;
    std::function<void(int)> on_particle_spawn_callback; // takes in id of spawn particle
    std::function<void(int)> on_particle_death_callback; // takes in id of dead particle

    float time_since_last_spawn;

    UniqueIDGenerator particle_uid_generator = UniqueIDGenerator();
};

#endif // PARTICLE_EMITTER_HPP
