#include "Camara.h"
#include "Configuracion.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

void Camara::orbitar(float deltaYaw, float deltaElevacion) {
    if (modo == ModoCamara::Seguimiento) offsetSeguimiento += deltaYaw;
    else yaw += deltaYaw;
    elevacion = std::clamp(elevacion + deltaElevacion,
                           Configuracion::CAM_ELEV_MIN, Configuracion::CAM_ELEV_MAX);
}

void Camara::acercar(float delta) {
    radio = std::clamp(radio - delta,
                       Configuracion::CAM_RADIO_MIN, Configuracion::CAM_RADIO_MAX);
}

void Camara::establecerRadio(float nuevoRadio) {
    radio = std::clamp(nuevoRadio, Configuracion::CAM_RADIO_MIN, Configuracion::CAM_RADIO_MAX);
}

void Camara::seguir(const glm::vec3& puntoObjetivo) {
    objetivoDeseado = puntoObjetivo;
}

void Camara::establecerDatosDron(float nuevoYawDron, const glm::vec3& nuevaVelocidad) {
    yawDron = nuevoYawDron;
    velocidadDron = nuevaVelocidad;
}

glm::vec3 Camara::posicionOrbitalDe(const glm::vec3& centro) const {
    if (modo == ModoCamara::Superior)
        return centro + glm::vec3(0.01f, Configuracion::CAM_RADIO_SUPERIOR, 0.01f);
    float y = glm::radians(yaw), e = glm::radians(elevacion);
    return centro + glm::vec3(radio * std::cos(e) * std::sin(y),
                              radio * std::sin(e),
                              radio * std::cos(e) * std::cos(y));
}

void Camara::actualizar(float dt) {
    if (dt <= 0.0f) return;

    // 1 - base^dt: la fraccion que se recorre en este frame. Con base = 0.001,
    // al cabo de un segundo solo queda sin recorrer el 0,1% de la distancia,
    // sea cual sea el numero de frames que hayan cabido en ese segundo.
    dt = std::min(dt, 0.05f);
    float t = 1.0f - std::pow(Configuracion::CAM_BASE_SUAVIZADO, dt);

    if (modo == ModoCamara::Seguimiento) {
        float objetivoYaw = yawDron + 180.0f + offsetSeguimiento;
        float diferencia = std::fmod(objetivoYaw - yaw + 540.0f, 360.0f) - 180.0f;
        // El giro deliberadamente va mas lento que el punto de enfoque: crea
        // el pequeno retraso cinematografico al cambiar de direccion.
        yaw += diferencia * (1.0f - std::exp(-2.8f * dt));
    }

    objetivo = glm::mix(objetivo, objetivoDeseado, t);
    // La orbita se calcula alrededor del objetivo YA suavizado: si se usara el
    // deseado, la posicion adelantaria a la mirada y la escena cabecearia.
    posicion = glm::mix(posicion, posicionOrbitalDe(objetivo), t);
}

void Camara::evitarTerreno(const Terreno& terreno) {
    float minimo = terreno.alturaEn(posicion.x, posicion.z) + Configuracion::CAM_ALTURA_SUELO;
    if (posicion.y < minimo) posicion.y = minimo;
}

void Camara::siguienteModo() {
    modo = static_cast<ModoCamara>((static_cast<int>(modo) + 1) % 3);
    if (modo == ModoCamara::Superior) elevacion = Configuracion::CAM_ELEV_MAX;
    else if (elevacion > 75.0f) elevacion = Configuracion::CAM_ELEV_INICIAL;
}

void Camara::recentrar() {
    offsetSeguimiento = 0.0f;
    if (modo == ModoCamara::Seguimiento) yaw = yawDron + 180.0f;
}

void Camara::saltarAObjetivo() {
    objetivo = objetivoDeseado;
    posicion = posicionOrbitalDe(objetivo);
}

glm::mat4 Camara::matrizVista() const {
    return glm::lookAt(posicion, objetivo, glm::vec3(0, 1, 0));
}

glm::mat4 Camara::matrizProyeccion(float aspecto) const {
    return glm::perspective(glm::radians(Configuracion::CAM_FOV), aspecto,
                            Configuracion::CAM_CERCANO, Configuracion::CAM_LEJANO);
}

glm::vec3 Camara::adelante() const {
    float y = glm::radians(yaw);
    return glm::normalize(glm::vec3(-std::sin(y), 0.0f, -std::cos(y)));
}

glm::vec3 Camara::derecha() const {
    float y = glm::radians(yaw);
    return glm::normalize(glm::vec3(std::cos(y), 0.0f, -std::sin(y)));
}
