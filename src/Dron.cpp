#include "Dron.h"
#include "Configuracion.h"
#include "EscalaMundo.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

void Dron::mover(const glm::vec3& direccion) {
    glm::vec3 plana(direccion.x, 0.0f, direccion.z);
    direccionEntrada = (glm::length(plana) > 1e-4f) ? glm::normalize(plana) : glm::vec3(0.0f);
}

void Dron::ajustarAltura(float eje) {
    ejeVertical = std::clamp(eje, -1.0f, 1.0f);
}

void Dron::actualizar(const Terreno& terreno, const EscalaMundo& escala, float dt) {
    if (dt <= 0.0f) return;
    dt = std::min(dt, 0.05f);   // evita saltos al mover o restaurar la ventana

    // ---- 1) Velocidad objetivo: SOLO entrada del usuario -------------------
    glm::vec3 objetivo = direccionEntrada * escala.velocidadMaxima;
    objetivo.y = ejeVertical * escala.velocidadVertical;
    const bool hayEntrada = glm::length(objetivo) > 1e-5f;

    // ---- 2) Aproximacion con ACELERACION LIMITADA --------------------------
    // Se avanza hacia el objetivo como mucho tasa*dt por frame. A diferencia de
    // un lerp exponencial, esto llega EXACTAMENTE al objetivo (y al cero) en
    // tiempo finito, que es lo que elimina el deslizamiento residual.
    const float tasa = hayEntrada ? escala.aceleracion : escala.desaceleracion;
    glm::vec3 delta = objetivo - velocidad;
    float distancia = glm::length(delta);
    float maximoCambio = tasa * dt;
    velocidad += (distancia > maximoCambio && distancia > 1e-8f)
               ? delta * (maximoCambio / distancia)
               : delta;

    // Zona muerta: por debajo del epsilon la velocidad se anula por completo.
    if (!hayEntrada && glm::length(velocidad) < escala.epsilonVelocidad)
        velocidad = glm::vec3(0.0f);

    // ---- 3) Integracion ----------------------------------------------------
    posicion += velocidad * dt;

    // ---- 4) Limites del mapa: se corta, no se rebota ----------------------
    const LimitesMundo& lim = terreno.obtenerLimites();
    const float margen = escala.margenMapa;
    float xAntes = posicion.x, zAntes = posicion.z;
    posicion.x = std::clamp(posicion.x, lim.minX + margen, lim.maxX - margen);
    posicion.z = std::clamp(posicion.z, lim.minZ + margen, lim.maxZ - margen);
    // Rebotar generaria movimiento no pedido por el usuario: se anula el eje.
    if (posicion.x != xAntes) velocidad.x = 0.0f;
    if (posicion.z != zAntes) velocidad.z = 0.0f;

    float distanciaBorde = std::min({posicion.x - lim.minX, lim.maxX - posicion.x,
                                     posicion.z - lim.minZ, lim.maxZ - posicion.z});
    cercaDelBorde = distanciaBorde < escala.diagonalTerreno * 0.05f;

    // ---- 5) Colision con el relieve: SOLO si esta por debajo del minimo ----
    const float suelo  = terreno.alturaEn(posicion.x, posicion.z);
    const float minimo = suelo + escala.alturaSegura;
    const float maximo = suelo + escala.alturaMaxima;
    if (posicion.y < minimo) {
        posicion.y = minimo;
        velocidad.y = std::max(0.0f, velocidad.y);
    } else if (posicion.y > maximo) {
        posicion.y = maximo;
        velocidad.y = std::min(0.0f, velocidad.y);
    }

    // ---- 6) Orientacion: interpolacion exponencial, nunca sobrepasa --------
    glm::vec2 horizontal(velocidad.x, velocidad.z);
    float rapidezHorizontal = glm::length(horizontal);
    // Solo gira si de verdad se desplaza: con el dron parado el yaw se congela.
    if (rapidezHorizontal > escala.velocidadMaxima * 0.04f) {
        float yawObjetivo = glm::degrees(std::atan2(velocidad.x, velocidad.z))
                          + Configuracion::OFFSET_YAW_DRON;
        float diferencia = std::fmod(yawObjetivo - yaw + 540.0f, 360.0f) - 180.0f;
        yaw += diferencia * (1.0f - std::exp(-Configuracion::SUAVIZADO_YAW * dt));
    }

    // Inclinacion proporcional a la fraccion de velocidad maxima en cada eje
    // del propio dron: adelante -> morro abajo, lateral -> alabeo al lado.
    float relativa = glm::clamp(rapidezHorizontal / std::max(1e-4f, escala.velocidadMaxima),
                                0.0f, 1.0f);
    glm::vec3 frente(std::sin(glm::radians(yaw)), 0.0f, std::cos(glm::radians(yaw)));
    glm::vec3 derecha(std::cos(glm::radians(yaw)), 0.0f, -std::sin(glm::radians(yaw)));
    float avance  = glm::dot(velocidad, frente)  / std::max(1e-4f, escala.velocidadMaxima);
    float lateral = glm::dot(velocidad, derecha) / std::max(1e-4f, escala.velocidadMaxima);

    float pitchObjetivo = -Configuracion::INCLINACION_MAX * glm::clamp(avance,  -1.0f, 1.0f) * relativa;
    float rollObjetivo  = -Configuracion::INCLINACION_MAX * glm::clamp(lateral, -1.0f, 1.0f);

    float estabiliza = 1.0f - std::exp(-Configuracion::SUAVIZADO_INCLINACION * dt);
    pitch = glm::mix(pitch, pitchObjetivo, estabiliza);
    roll  = glm::mix(roll,  rollObjetivo,  estabiliza);
    // Sin entrada, la orientacion converge a neutro sin oscilar: por debajo de
    // una decima de grado se fija a cero y deja de haber cambio visible.
    if (!hayEntrada) {
        if (std::abs(pitch) < 0.05f) pitch = 0.0f;
        if (std::abs(roll)  < 0.05f) roll  = 0.0f;
    }

    alturaSobreTerreno = posicion.y - suelo;
}
