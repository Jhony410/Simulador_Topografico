#pragma once

enum class EstadoAplicacion {
    Loading,
    Intro,
    Playing,
    Paused,
    MissionComplete,
    Error
};

inline const char* nombreEstado(EstadoAplicacion estado) {
    switch (estado) {
        case EstadoAplicacion::Loading:         return "CARGANDO";
        case EstadoAplicacion::Intro:           return "LISTO";
        case EstadoAplicacion::Playing:         return "EN EXPLORACION";
        case EstadoAplicacion::Paused:          return "PAUSA";
        case EstadoAplicacion::MissionComplete: return "MISION COMPLETADA";
        case EstadoAplicacion::Error:           return "ERROR";
    }
    return "DESCONOCIDO";
}
