#pragma once
#include <glm/glm.hpp>
#include <vector>

class Terreno;

struct ResultadoMedicion {
    float distanciaHorizontal = 0.0f;
    float diferenciaElevacion = 0.0f;
    float distancia3D = 0.0f;
    float pendientePorcentual = 0.0f;
    float anguloGrados = 0.0f;
    float areaProyectada = 0.0f;
};

class SistemaMedicion {
public:
    void alternar() { activo = !activo; }
    bool estaActivo() const { return activo; }
    void limpiar();
    void agregarPunto(const glm::vec3& punto);
    bool seleccionarTerrenoPorRayo(const glm::vec3& origen, const glm::vec3& direccion,
                                   const Terreno& terreno);
    ResultadoMedicion calcular() const;
    const std::vector<glm::vec3>& obtenerPuntos() const { return puntos; }

private:
    bool activo = false;
    std::vector<glm::vec3> puntos;
};
