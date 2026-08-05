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
    // Desplaza en el plano XZ segun una direccion ya normalizada y orienta el
    // morro hacia donde avanza.
    void mover(const glm::vec3& direccion, float velocidadLineal, float dt);

    // Sube o baja (delta positivo = subir).
    void ajustarAltura(float delta);

    // Recalcula datos derivados (altura sobre el relieve) tras mover.
    void actualizar(const Terreno& terreno, float dt);

    void establecerPosicion(const glm::vec3& p) { posicion = p; }
    void establecerYaw(float grados)            { yaw = grados; }

    const glm::vec3& obtenerPosicion() const { return posicion; }
    const glm::vec3& obtenerVelocidad() const { return velocidad; }
    float obtenerYaw()   const { return yaw; }
    float obtenerPitch() const { return pitch; }
    float obtenerRoll()  const { return roll; }
    float obtenerAlturaSobreTerreno() const { return alturaSobreTerreno; }

private:
    glm::vec3 posicion{0.0f};
    glm::vec3 velocidad{0.0f};
    float yaw   = 0.0f;   // grados alrededor de Y
    float pitch = 0.0f;
    float roll  = 0.0f;
    float alturaSobreTerreno = 0.0f;
};
