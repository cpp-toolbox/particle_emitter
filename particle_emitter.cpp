#include "particle_emitter.hpp"
#include <algorithm>

Particle::Particle(float lifespan_seconds, const glm::vec3 &initial_velocity,
                   std::function<glm::vec3(float, float)> velocity_change_func,
                   std::function<float(float)> scaling_func, std::function<float(float)> rotation_func, int id)
    : lifespan_seconds(lifespan_seconds), age_seconds(0.0), velocity(initial_velocity),
      character_velocity_change_function(velocity_change_func), character_scaling_function(scaling_func),
      character_rotation_degrees_function(rotation_func), transform(Transform()), id(id) {
    transform.scale = glm::vec3(character_scaling_function(0.0f));
}

void Particle::update(float delta_time, glm::mat4 world_to_clip) {
    age_seconds += delta_time;
    float life_percentage = age_seconds / lifespan_seconds;

    if (life_percentage >= 1.0f) {
        age_seconds = lifespan_seconds; // Mark as expired
        return;
    }

    velocity += character_velocity_change_function(life_percentage, delta_time);
    transform.position += velocity * delta_time;
    transform.scale = glm::vec3(character_scaling_function(life_percentage));
    transform.rotation.z = character_rotation_degrees_function(life_percentage);

    distance_to_camera = (world_to_clip * glm::vec4(transform.position, 1)).z;
}

bool Particle::operator<(const Particle &that) const { return this->distance_to_camera > that.distance_to_camera; }

bool Particle::is_alive() const { return age_seconds < lifespan_seconds; }

ParticleEmitter::ParticleEmitter(std::function<float()> lifespan_func, std::function<glm::vec3()> initial_velocity_func,
                                 std::function<glm::vec3(float, float)> velocity_change_func,
                                 std::function<float(float)> scaling_func, std::function<float(float)> rotation_func,
                                 std::function<float()> spawn_delay_func,
                                 std::function<void(int)> on_particle_spawn_callback,
                                 std::function<void(int)> on_particle_death_callback)
    : lifespan_func(lifespan_func), initial_velocity_func(initial_velocity_func),
      velocity_change_func(velocity_change_func), scaling_func(scaling_func), rotation_func(rotation_func),
      spawn_delay_func(spawn_delay_func), time_since_last_spawn(0.0f),
      on_particle_spawn_callback(on_particle_spawn_callback), on_particle_death_callback(on_particle_death_callback) {}

ParticleEmitter::~ParticleEmitter() {
    for (auto &particle : particles) {
        particle_uid_generator.reclaim_id(particle.id);
        on_particle_death_callback(particle.id);
    }

    particles.clear();
}

void ParticleEmitter::remove_dead_particles() {
    for (auto it = particles.begin(); it != particles.end();) {
        if (!it->is_alive()) {
            particle_uid_generator.reclaim_id(it->id);
            on_particle_death_callback(it->id);
            it = particles.erase(it); // erase returns the next iterator
        } else {
            ++it;
        }
    }
}

void ParticleEmitter::update(float delta_time, glm::mat4 world_to_clip) {
    time_since_last_spawn += delta_time;
    try_to_spawn_new_particle();
    remove_dead_particles();

    for (Particle &particle : particles) {
        particle.update(delta_time, world_to_clip);
    }
}

void ParticleEmitter::try_to_spawn_new_particle() {
    float spawn_delay = spawn_delay_func();
    if (time_since_last_spawn >= spawn_delay) {

        auto new_particle = spawn_particle();

        on_particle_spawn_callback(new_particle.id);

        particles.emplace_back(new_particle);
        time_since_last_spawn = 0.0f;
    }
}

Particle ParticleEmitter::spawn_particle() {
    float lifespan = lifespan_func();
    glm::vec3 velocity = initial_velocity_func();
    Particle particle(lifespan, velocity, velocity_change_func, scaling_func, rotation_func,
                      particle_uid_generator.get_id());
    particle.transform.position = transform.position;
    return particle;
}

std::vector<Particle> ParticleEmitter::get_particles_sorted_by_distance() const {
    std::vector<Particle> sorted_particles = particles;
    std::sort(sorted_particles.begin(), sorted_particles.end());
    return sorted_particles;
}
