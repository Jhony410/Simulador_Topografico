#include "Dron.h"
#include "Configuracion.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

void Dron::mover(const glm::vec3& direccion, float velocidadLineal, float dt) {
    (void)dt;
    velocidadObjetivo = glm::length(direccion) > 1e-4f
                      ? glm::normalize(direccion) * velocidadLineal
                      : glm::vec3(0.0f);
    velocidadObjetivo.y = 0.0f;
}

void Dron::ajustarAltura(float delta) {
    // El controlador entrega velocidad * dt por compatibilidad con la interfaz
    // anterior. Recuperarla permite filtrar tambien el eje vertical.
    velocidadVerticalObjetivo += delta;
}

void Dron::actualizar(const Terreno& terreno, float dt) {
    if (dt <= 0.0f) return;
    dt = std::min(dt, 0.05f); // evita saltos grandes al mover/restaurar ventana
    tiempoVuelo += dt;

    // ajustarAltura recibe un desplazamiento solicitado por frame. Se convierte
    // a velocidad antes de la interpolacion; si no hubo tecla vuelve a cero.
    float objetivoY = velocidadVerticalObjetivo / dt;
    velocidadVerticalObjetivo = 0.0f;
    glm::vec3 objetivo = velocidadObjetivo;
    objetivo.y = objetivoY;

    float constante = glm::length(objetivo) > 0.01f
                    ? Configuracion::ACELERACION_DRON : Configuracion::FRENO_DRON;
    float mezcla = 1.0f - std::exp(-constante * dt);
    velocidad = glm::mix(velocidad, objetivo, mezcla);
    if (glm::length(velocidad) < 0.015f && glm::length(objetivo) < 0.01f)
        velocidad = glm::vec3(0.0f);
    posicion += velocidad * dt;

    const LimitesMundo& lim = terreno.obtenerLimites();
    float margen = Configuracion::MARGEN_MAPA;
    float xAntes = posicion.x, zAntes = posicion.z;
    posicion.x = std::clamp(posicion.x, lim.minX + margen, lim.maxX - margen);
    posicion.z = std::clamp(posicion.z, lim.minZ + margen, lim.maxZ - margen);
    if (posicion.x != xAntes) velocidad.x *= -0.15f;
    if (posicion.z != zAntes) velocidad.z *= -0.15f;
    float distanciaBorde = std::min({posicion.x - lim.minX, lim.maxX - posicion.x,
                                     posicion.z - lim.minZ, lim.maxZ - posicion.z});
    cercaDelBorde = distanciaBorde < 7.0f;

    float suelo = terreno.alturaEn(posicion.x, posicion.z);
    float minimo = suelo + Configuracion::ALTURA_MINIMA;
    float maximo = suelo + Configuracion::ALTURA_MAXIMA;
    if (posicion.y < minimo) {
        posicion.y = minimo;
        velocidad.y = std::max(0.0f, velocidad.y);
    }
    if (posicion.y > maximo) { posicion.y = maximo; velocidad.y = std::min(0.0f, velocidad.y); }

    glm::vec2 horizontal(velocidad.x, velocidad.z);
    if (glm::length(horizontal) > 0.4f) {
        float yawObjetivo = glm::degrees(std::atan2(velocidad.x, velocidad.z)) + Configuracion::OFFSET_YAW_DRON;
        float diferencia = std::fmod(yawObjetivo - yaw + 540.0f, 360.0f) - 180.0f;
        yaw += diferencia * (1.0f - std::exp(-7.0f * dt));
    }
    float rapidezRelativa = glm::clamp(glm::length(horizontal) / Configuracion::VEL_DRON, 0.0f, 1.0f);
    float pitchObjetivo = -Configuracion::INCLINACION_MAX * rapidezRelativa;
    glm::vec3 derecha(std::cos(glm::radians(yaw)), 0.0f, -std::sin(glm::radians(yaw)));
    float lateral = glm::dot(velocidad, derecha) / std::max(1.0f, Configuracion::VEL_DRON);
    float rollObjetivo = -Configuracion::INCLINACION_MAX * glm::clamp(lateral, -1.0f, 1.0f);
    float estabiliza = 1.0f - std::exp(-5.0f * dt);
    pitch = glm::mix(pitch, pitchObjetivo, estabiliza);
    roll  = glm::mix(roll, rollObjetivo, estabiliza);
    flotacion = rapidezRelativa < 0.04f ? std::sin(tiempoVuelo * 1.8f) * 0.16f : 0.0f;

    alturaSobreTerreno = posicion.y - terreno.alturaEn(posicion.x, posicion.z);
}
