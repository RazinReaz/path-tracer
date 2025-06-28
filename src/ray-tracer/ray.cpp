#include "ray.h"

Ray::Ray()
    : origin(0.0f, 0.0f, 0.0f), direction(0.0f, 0.0f, 0.0f)
{
    info.hit = false;
    info.t = 10000.0;
    info.norm = glm::vec3(0.0f, 0.0f, 0.0f);
}

Ray::Ray(const glm::vec3 &origin, const glm::vec3 &direction)
    : origin(origin), direction(glm::normalize(direction))
{
    info.hit = false;
    info.t = 10000.0;
    info.norm = glm::vec3(0.0f, 0.0f, 0.0f);
}

void
Ray::set_hit(const float& distance, const glm::vec3& normal) {
    if (distance < 0 || distance > this->info.t)
        return;

    this->info.t = distance;
    this->info.hit = true;
    this->info.norm = normal;
}