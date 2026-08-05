#pragma once
#include <glm/glm.hpp>
#include <vector>

// ============================================================================
//  MODELO: curvas de nivel (isolineas).
//  En la FASE 1 solo se define el contenedor; la generacion por Marching
//  Squares y el encadenado por grafo llegan en la FASE 4. Se declara ahora
//  para que la Vista y el resto del Modelo ya conozcan el tipo de dato.
// ============================================================================

// Una isolinea: puntos en el plano XZ del mundo, todos a la misma altura.
struct Polilinea {
    std::vector<glm::vec2> puntos;
    float altura   = 0.0f;
    bool  cerrada  = false;
};

class CurvasNivel {
public:
    void limpiar();
    void agregar(Polilinea curva);
    void establecerRango(float minimo, float maximo);

    const std::vector<Polilinea>& obtenerCurvas() const { return curvas; }
    float obtenerAlturaMinima() const { return alturaMinima; }
    float obtenerAlturaMaxima() const { return alturaMaxima; }

    // Altura normalizada 0..1, para mapear a la rampa azul -> rojo.
    float normalizar(float altura) const;

    bool vacio() const { return curvas.empty(); }
    std::size_t cantidad() const { return curvas.size(); }

private:
    std::vector<Polilinea> curvas;
    float alturaMinima = 0.0f;
    float alturaMaxima = 1.0f;
};
