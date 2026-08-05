#include "Dron.h"
#include "Configuracion.h"
#include "Terreno.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

void Dron::mover(const glm::vec3& direccion, float velocidadLineal, float dt) {
    if (glm::length(direccion) <= 1e-4f) { velocidad = glm::vec3(0.0f); return; }

    glm::vec3 d = glm::normalize(direccion);
    velocidad = d * velocidadLineal;
    posicion += velocidad * dt;

    // El morro apunta a donde se avanza; atan2(x, z) porque -Z es "adelante".
    yaw = glm::degrees(std::atan2(d.x, d.z)) + Configuracion::OFFSET_YAW_DRON;
}

void Dron::ajustarAltura(float delta) {
    posicion.y += delta;
}

void Dron::actualizar(const Terreno& terreno, float dt) {
    (void)dt;
    alturaSobreTerreno = posicion.y - terreno.alturaEn(posicion.x, posicion.z);
}
