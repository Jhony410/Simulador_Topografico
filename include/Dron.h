#pragma once
#include <glm/glm.hpp>

class Terreno;

// ============================================================================
//  MODELO: estado del dron.
//  Solo posicion, orientacion y velocidad. No sabe como se dibuja ni con que
//  malla; de eso se encarga VistaDron leyendo esta informacion.
// ============================================================================
class Dron {
public:
    // Define la velocidad horizontal deseada. La integracion suave ocurre en
    // actualizar(), para que aceleracion y frenado dependan de deltaTime.
    void mover(const glm::vec3& direccion, float velocidadLineal, float dt);

    // Sube o baja (delta positivo = subir).
    void ajustarAltura(float delta);

    // Recalcula datos derivados (altura sobre el relieve) tras mover.
    void actualizar(const Terreno& terreno, float dt);

    void establecerPosicion(const glm::vec3& p) {
        posicion = p;
        velocidad = velocidadObjetivo = glm::vec3(0.0f);
        velocidadVerticalObjetivo = 0.0f;
        pitch = roll = 0.0f;
    }
    void establecerYaw(float grados)            { yaw = grados; }

    const glm::vec3& obtenerPosicion() const { return posicion; }
    const glm::vec3& obtenerVelocidad() const { return velocidad; }
    float obtenerYaw()   const { return yaw; }
    float obtenerPitch() const { return pitch; }
    float obtenerRoll()  const { return roll; }
    float obtenerAlturaSobreTerreno() const { return alturaSobreTerreno; }
    float obtenerRapidez() const { return glm::length(velocidad); }
    float obtenerFlotacion() const { return flotacion; }
    bool  estaCercaDelBorde() const { return cercaDelBorde; }
    bool  estaDemasiadoBajo() const { return alturaSobreTerreno < 4.0f; }

private:
    glm::vec3 posicion{0.0f};
    glm::vec3 velocidad{0.0f};
    glm::vec3 velocidadObjetivo{0.0f};
    float velocidadVerticalObjetivo = 0.0f;
    float yaw   = 0.0f;   // grados alrededor de Y
    float pitch = 0.0f;
    float roll  = 0.0f;
    float alturaSobreTerreno = 0.0f;
    float tiempoVuelo = 0.0f;
    float flotacion = 0.0f;
    bool cercaDelBorde = false;
};
