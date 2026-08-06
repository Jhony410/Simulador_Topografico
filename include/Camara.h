#pragma once
#include <glm/glm.hpp>

class Terreno;

enum class ModoCamara {
    Orbital,
    Seguimiento,
    Superior
};

inline const char* nombreModoCamara(ModoCamara modo) {
    switch (modo) {
        case ModoCamara::Orbital:     return "ORBITAL LIBRE";
        case ModoCamara::Seguimiento: return "SEGUIMIENTO";
        case ModoCamara::Superior:    return "INSPECCION SUPERIOR";
    }
    return "DESCONOCIDA";
}

// ============================================================================
//  VISTA: camara orbital con seguimiento suave del dron.
//
//  El objetivo y la posicion no saltan al sitio nuevo cada frame: se interpolan
//  con un factor 1 - base^dt. Esa forma es INDEPENDIENTE DEL FRAMERATE (a 30 o
//  a 300 FPS la camara tarda lo mismo en llegar), a diferencia del clasico
//  lerp(actual, deseado, k) con k fijo, que a mas FPS persigue mas rapido.
//  'base' es la fraccion de distancia que aun quedaria por recorrer al cabo de
//  un segundo, de ahi que valga 0.001.
// ============================================================================
class Camara {
public:
    void orbitar(float deltaYaw, float deltaElevacion);
    void acercar(float delta);                     // delta > 0 = acercarse
    void establecerRadio(float radio);

    void seguir(const glm::vec3& puntoObjetivo);   // fija a donde QUIERE ir
    void establecerDatosDron(float yawDron, const glm::vec3& velocidadDron);
    void actualizar(float dt);                     // aplica el suavizado
    void saltarAObjetivo();                        // sin interpolar (cambio de mapa)
    void evitarTerreno(const Terreno& terreno);

    void siguienteModo();
    void recentrar();
    ModoCamara obtenerModo() const { return modo; }

    glm::mat4 matrizVista() const;
    glm::mat4 matrizProyeccion(float aspecto) const;

    const glm::vec3& obtenerPosicion() const { return posicion; }
    const glm::vec3& obtenerObjetivo() const { return objetivo; }

    // Ejes de movimiento en el plano XZ, alineados con la orientacion actual.
    // Los usa el Controlador para que "adelante" sea siempre adelante en pantalla.
    glm::vec3 adelante() const;
    glm::vec3 derecha() const;

    float obtenerYaw()       const { return yaw; }
    float obtenerElevacion() const { return elevacion; }
    float obtenerRadio()     const { return radio; }

private:
    glm::vec3 posicionOrbitalDe(const glm::vec3& centro) const;

    float yaw       = 45.0f;   // grados
    float elevacion = 14.0f;   // grados: angulo bajo, casi a ras del relieve
    float radio     = 46.0f;
    float yawDron   = 0.0f;
    float yawSeguimiento = 180.0f;
    float offsetSeguimiento = 0.0f;
    glm::vec3 velocidadDron{0.0f};
    ModoCamara modo = ModoCamara::Seguimiento;

    glm::vec3 objetivoDeseado{0.0f};   // donde esta el dron ahora mismo
    glm::vec3 objetivo{0.0f};          // hacia donde mira la camara (suavizado)
    glm::vec3 posicion{0.0f};          // donde esta la camara (suavizado)
};
