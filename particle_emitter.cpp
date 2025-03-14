#include "particle_emitter.hpp"
#include <algorithm>

Particle::Particle(float lifespan_seconds, const glm::vec3 &initial_velocity,
                   std::function<glm::vec3(float, float)> velocity_change_func,
                   std::function<float(float)> scaling_func, std::function<float(float)> rotation_func, int id)
    : lifespan_seconds(lifespan_seconds), age_seconds(0.0), velocity(initial_velocity),
      character_velocity_change_function(velocity_change_func), character_scaling_function(scaling_func),
      character_rotation_degrees_function(rotation_func), transform(Transform()), id(id) {
    transform.set_scale(glm::vec3(character_scaling_function(0.0f)));
}

void Particle::update(float delta_time, glm::mat4 world_to_clip) {
    age_seconds += delta_time;
    float life_percentage = age_seconds / lifespan_seconds;

    if (life_percentage >= 1.0f) {
        age_seconds = lifespan_seconds; // Mark as expired
        return;
    }

    velocity += character_velocity_change_function(life_percentage, delta_time);
    transform.add_position(velocity * delta_time);
    transform.set_scale(glm::vec3(character_scaling_function(life_percentage)));
    transform.set_rotation_roll(character_rotation_degrees_function(life_percentage));

    distance_to_camera = (world_to_clip * glm::vec4(transform.get_translation(), 1)).z;
}

bool Particle::operator<(const Particle &that) const { return this->distance_to_camera > that.distance_to_camera; }

bool Particle::is_alive() const { return age_seconds < lifespan_seconds; }

ParticleEmitter::ParticleEmitter(std::function<float()> lifespan_func, std::function<glm::vec3()> initial_velocity_func,
                                 std::function<glm::vec3(float, float)> velocity_change_func,
                                 std::function<float(float)> scaling_func, std::function<float(float)> rotation_func,
                                 std::function<float()> spawn_delay_func,
                                 std::function<void(int, int)> on_particle_spawn_callback,
                                 std::function<void(int, int)> on_particle_death_callback, int id, double rate_limit_hz)
    : lifespan_func(lifespan_func), initial_velocity_func(initial_velocity_func),
      velocity_change_func(velocity_change_func), scaling_func(scaling_func), rotation_func(rotation_func),
      spawn_delay_func(spawn_delay_func), time_since_last_spawn(0.0f),
      on_particle_spawn_callback(on_particle_spawn_callback), on_particle_death_callback(on_particle_death_callback),
      id(id), rate_limiter(rate_limit_hz) {}

ParticleEmitter::~ParticleEmitter() {
    for (auto &particle : particles) {
        particle_uid_generator.reclaim_id(particle.id);
        on_particle_death_callback(id, particle.id);
    }

    particles.clear();
}

void ParticleEmitter::remove_dead_particles() {
    for (auto it = particles.begin(); it != particles.end();) {
        if (!it->is_alive()) {
            particle_uid_generator.reclaim_id(it->id);
            on_particle_death_callback(id, it->id);
            it = particles.erase(it); // erase returns the next iterator
        } else {
            ++it;
        }
    }
}

void ParticleEmitter::update(float delta_time, glm::mat4 world_to_clip) {
    // TODO: this variable is bad...
    time_since_last_spawn += delta_time;

    if (rate_limiter.attempt_to_run()) {
        try_to_spawn_new_particle();
        remove_dead_particles();

        for (Particle &particle : particles) {
            particle.update(rate_limiter.get_last_processed_time(), world_to_clip);
        }
        particles_require_sorting = true;
    }
}

// TODO: this logic is bad, we need to create new particles until we use up all of our time we have availble to use.
// aka turn the if into a while...
void ParticleEmitter::try_to_spawn_new_particle() {
    float spawn_delay = spawn_delay_func();
    if (time_since_last_spawn >= spawn_delay) {

        auto new_particle = spawn_particle();

        on_particle_spawn_callback(id, new_particle.id);

        particles.emplace_back(new_particle);
        time_since_last_spawn = 0.0f;
    }
}

Particle ParticleEmitter::spawn_particle() {
    float lifespan = lifespan_func();
    glm::vec3 velocity = initial_velocity_func();
    Particle particle(lifespan, velocity, velocity_change_func, scaling_func, rotation_func,
                      particle_uid_generator.get_id());
    particle.transform.set_position(transform.get_translation());
    return particle;
}

std::vector<Particle> ParticleEmitter::get_particles_sorted_by_distance() {

    if (particles_require_sorting) {
        std::vector<Particle> particles_copy = particles;
        std::sort(particles_copy.begin(), particles_copy.end());
        last_sorted_particles = particles_copy;
        particles_require_sorting = false;
    }

    return last_sorted_particles;
}
