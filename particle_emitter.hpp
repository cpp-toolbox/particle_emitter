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
             std::function<float(float)> rotation_func, Transform emitter_transform);

    void update(float delta_time, glm::mat4 world_to_clip);

    bool operator<(const Particle &that) const;

    bool is_alive() const;

    Transform transform;
    Transform emitter_transform;

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
    Transform transform;

    ParticleEmitter(std::function<float()> lifespan_func, std::function<glm::vec3()> initial_velocity_func,
                    std::function<glm::vec3(float, float)> velocity_change_func,
                    std::function<float(float)> scaling_func, std::function<float(float)> rotation_func,
                    std::function<float()> spawn_delay_func, unsigned int max_particles, Transform initial_transform);

    void update(float delta_time, glm::mat4 world_to_clip);

    void stop_emitting_particles();
    void resume_emitting_particles();

    std::vector<Particle> get_particles_sorted_by_distance();
    std::vector<Particle> particles;

  private:
    unsigned int max_particles;
    unsigned int last_used_particle;
    float time_since_last_spawn;
    bool currently_producing_particles = true;

    std::function<float()> lifespan_func;
    std::function<glm::vec3()> initial_velocity_func;
    std::function<glm::vec3(float, float)> velocity_change_func;
    std::function<float(float)> scaling_func;
    std::function<float(float)> rotation_func;
    std::function<float()> spawn_delay_func;

    unsigned int find_unused_particle();
    void respawn_particle(Particle &particle);
};

#endif // PARTICLE_EMITTER_HPP
