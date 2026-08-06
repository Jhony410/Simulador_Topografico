#include "SistemaMedicion.h"
#include "Terreno.h"

#include <algorithm>
#include <cmath>

void SistemaMedicion::limpiar() { puntos.clear(); }

void SistemaMedicion::agregarPunto(const glm::vec3& punto) {
    if (puntos.size() >= 12) puntos.erase(puntos.begin());
    puntos.push_back(punto);
}

bool SistemaMedicion::seleccionarTerrenoPorRayo(const glm::vec3& origen,
                                                const glm::vec3& direccion,
                                                const Terreno& terreno) {
    if (!terreno.estaCargado() || direccion.y >= -1e-4f) return false;
    const LimitesMundo& lim = terreno.obtenerLimites();
    float maximo = 2000.0f;
    float anteriorT = 0.0f;
    float anteriorD = origen.y - terreno.alturaEn(origen.x, origen.z);
    const int pasos = 320;
    for (int i = 1; i <= pasos; ++i) {
        float t = maximo * static_cast<float>(i) / pasos;
        glm::vec3 p = origen + direccion * t;
        if (p.x < lim.minX || p.x > lim.maxX || p.z < lim.minZ || p.z > lim.maxZ) {
            anteriorT = t;
            anteriorD = p.y - terreno.alturaEn(p.x, p.z);
            continue;
        }
        float d = p.y - terreno.alturaEn(p.x, p.z);
        if (d <= 0.0f && anteriorD > 0.0f) {
            float a = anteriorT, b = t;
            for (int k = 0; k < 12; ++k) {
                float medio = (a + b) * 0.5f;
                glm::vec3 q = origen + direccion * medio;
                if (q.y > terreno.alturaEn(q.x, q.z)) a = medio; else b = medio;
            }
            glm::vec3 impacto = origen + direccion * ((a + b) * 0.5f);
            impacto.y = terreno.alturaEn(impacto.x, impacto.z);
            agregarPunto(impacto);
            return true;
        }
        anteriorT = t;
        anteriorD = d;
    }
    return false;
}

ResultadoMedicion SistemaMedicion::calcular() const {
    ResultadoMedicion r;
    if (puntos.size() >= 2) {
        glm::vec3 d = puntos[1] - puntos[0];
        r.distanciaHorizontal = glm::length(glm::vec2(d.x, d.z));
        r.diferenciaElevacion = d.y;
        r.distancia3D = glm::length(d);
        if (r.distanciaHorizontal > 1e-5f) {
            r.pendientePorcentual = (d.y / r.distanciaHorizontal) * 100.0f;
            r.anguloGrados = glm::degrees(std::atan2(d.y, r.distanciaHorizontal));
        }
    }
    if (puntos.size() >= 3) {
        double suma = 0.0;
        for (std::size_t i = 0; i < puntos.size(); ++i) {
            const glm::vec3& a = puntos[i];
            const glm::vec3& b = puntos[(i + 1) % puntos.size()];
            suma += static_cast<double>(a.x) * b.z - static_cast<double>(b.x) * a.z;
        }
        r.areaProyectada = static_cast<float>(std::abs(suma) * 0.5);
    }
    return r;
}
