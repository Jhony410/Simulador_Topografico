#pragma once
#include <glm/glm.hpp>

class Terreno;
struct EscalaMundo;

// ============================================================================
//  MODELO: estado del dron.
//
//  El movimiento es puramente dirigido por la entrada del usuario:
//
//      entrada  -> velocidadObjetivo = normalize(direccion) * velocidadMaxima
//      sin entrada -> velocidadObjetivo = 0
//      velocidad -> se aproxima a velocidadObjetivo con ACELERACION limitada
//      posicion  -> posicion + velocidad * dt
//
//  No hay flotacion automatica, ni oscilaciones, ni fuerzas aleatorias, ni
//  rutas. La posicion solo puede cambiar por: (1) entrada del usuario,
//  (2) correccion de colision con el relieve, (3) limites del mapa.
//
//  La zona muerta pone la velocidad a CERO EXACTO cuando ya es despreciable,
//  para que no quede deslizamiento residual al soltar las teclas.
// ============================================================================
class Dron {
public:
    // Direccion deseada en XZ (sin normalizar). Vector nulo = sin entrada.
    void mover(const glm::vec3& direccion);

    // Eje vertical: -1 bajar, 0 nada, +1 subir. Se reinicia cada frame.
    void ajustarAltura(float eje);

    // Integra velocidad y posicion, resuelve colision y actualiza orientacion.
    void actualizar(const Terreno& terreno, const EscalaMundo& escala, float dt);

    void establecerPosicion(const glm::vec3& p) {
        posicion = p;
        velocidad = glm::vec3(0.0f);
        direccionEntrada = glm::vec3(0.0f);
        ejeVertical = 0.0f;
        pitch = roll = 0.0f;
    }
    void establecerYaw(float grados) { yaw = grados; }

    const glm::vec3& obtenerPosicion()  const { return posicion; }
    const glm::vec3& obtenerVelocidad() const { return velocidad; }
    float obtenerYaw()   const { return yaw; }
    float obtenerPitch() const { return pitch; }
    float obtenerRoll()  const { return roll; }
    float obtenerAlturaSobreTerreno() const { return alturaSobreTerreno; }
    float obtenerRapidez() const { return glm::length(velocidad); }
    bool  estaCercaDelBorde() const { return cercaDelBorde; }
    bool  estaEnMovimiento()  const { return glm::length(velocidad) > 1e-5f; }

private:
    glm::vec3 posicion{0.0f};
    glm::vec3 velocidad{0.0f};
    glm::vec3 direccionEntrada{0.0f};  // XZ, la fija el Controlador cada frame
    float ejeVertical = 0.0f;          // -1..+1, la fija el Controlador cada frame
    float yaw   = 0.0f;                // grados alrededor de Y
    float pitch = 0.0f;
    float roll  = 0.0f;
    float alturaSobreTerreno = 0.0f;
    bool  cercaDelBorde = false;
};
