#include "particle_emitter.hpp"

Particle::Particle(float lifespan_seconds, const glm::vec3 &initial_velocity,
                   std::function<glm::vec3(float, float)> velocity_change_func,
                   std::function<float(float)> scaling_func, std::function<float(float)> rotation_func,
                   Transform &emitter_transform, int id)
    : lifespan_seconds(lifespan_seconds), age_seconds(0.0), velocity(initial_velocity),
      emitter_transform(emitter_transform), character_velocity_change_function(velocity_change_func),
      character_scaling_function(scaling_func), character_rotation_degrees_function(rotation_func),
      transform(Transform()), id(id) {
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
                                 std::function<float()> spawn_delay_func, unsigned int max_particles,
                                 Transform initial_transform)
    : lifespan_func(lifespan_func), initial_velocity_func(initial_velocity_func),
      velocity_change_func(velocity_change_func), scaling_func(scaling_func), rotation_func(rotation_func),
      spawn_delay_func(spawn_delay_func), max_particles(max_particles), last_used_particle(0),
      time_since_last_spawn(0.0f) {

    transform = initial_transform;

    particles.reserve(max_particles);
    for (unsigned int i = 0; i < max_particles; ++i) {
        particles.emplace_back(0, initial_velocity_func(), velocity_change_func, scaling_func, rotation_func, transform,
                               UniqueIDGenerator::generate());
    }
}

void ParticleEmitter::update(float delta_time, glm::mat4 world_to_clip) {

    time_since_last_spawn += delta_time;

    float spawn_delay = spawn_delay_func();

    if (currently_producing_particles) {
        if (time_since_last_spawn >= spawn_delay) {
            unsigned int unused_particle = find_unused_particle();
            respawn_particle(particles[unused_particle]);
            time_since_last_spawn = 0.0f; // Reset spawn timer
        }
    }

    for (Particle &particle : particles) {
        if (particle.is_alive()) {
            particle.update(delta_time, world_to_clip);
        }
    }
}

void ParticleEmitter::stop_emitting_particles() { currently_producing_particles = false; }
void ParticleEmitter::resume_emitting_particles() { currently_producing_particles = true; }

std::vector<Particle> ParticleEmitter::get_particles_sorted_by_distance() {
    std::vector<Particle> sorted_particles = particles;
    std::sort(sorted_particles.begin(), sorted_particles.end());
    return sorted_particles;
}

unsigned int ParticleEmitter::find_unused_particle() {
    for (unsigned int i = last_used_particle; i < max_particles; ++i) {
        if (!particles[i].is_alive()) {
            last_used_particle = i;
            return i;
        }
    }

    for (unsigned int i = 0; i < last_used_particle; ++i) {
        if (!particles[i].is_alive()) {
            last_used_particle = i;
            return i;
        }
    }

    last_used_particle = 0;
    return 0;
}

void ParticleEmitter::respawn_particle(Particle &particle) {
    float lifespan = lifespan_func();
    glm::vec3 velocity = initial_velocity_func();

    // use the same id as its the "same object"
    particle = Particle(lifespan, velocity, velocity_change_func, scaling_func, rotation_func, transform, particle.id);
    particle.transform = particle.emitter_transform;
}
