#pragma once
#include <glm/glm.hpp>

enum class EstadoPuntoEscaneo {
    Pendiente,
    Cercano,
    Escaneando,
    Completado
};

struct PuntoEscaneo {
    int id = 0;
    glm::vec3 posicion{0.0f};
    float radio = 5.5f;
    float progreso = 0.0f;
    EstadoPuntoEscaneo estado = EstadoPuntoEscaneo::Pendiente;
};
