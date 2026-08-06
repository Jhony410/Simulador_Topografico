#pragma once

enum class ModoVisualizacion {
    Wireframe,
    Puntos,
    WireframePuntos,
    SuperficieWireframe,
    Elevacion,
    Escaneo
};

inline const char* nombreModoVisualizacion(ModoVisualizacion modo) {
    switch (modo) {
        case ModoVisualizacion::Wireframe:           return "WIREFRAME";
        case ModoVisualizacion::Puntos:               return "NUBE DE PUNTOS";
        case ModoVisualizacion::WireframePuntos:      return "WIREFRAME + PUNTOS";
        case ModoVisualizacion::SuperficieWireframe:  return "SUPERFICIE + WIREFRAME";
        case ModoVisualizacion::Elevacion:            return "MAPA DE ELEVACION";
        case ModoVisualizacion::Escaneo:              return "ESCANEO";
    }
    return "DESCONOCIDO";
}

struct Ajustes {
    // Sensibilidad baja: es una de las causas de que el vuelo se sintiera
    // nervioso. En grados de orbita por pixel de arrastre.
    float sensibilidadMouse = 0.10f;
    float multiplicadorVelocidadDron = 1.0f;
    float intensidadPuntos = 0.70f;
    int   densidadWireframe = 2; // 0=baja, 1=media, 2=alta
    bool  curvasNivel = true;
    bool  particulas = true;
    bool  minimapa = true;
    bool  controles = true;
    bool  rutaVuelo = false;     // la guia al objetivo distrae en vuelo libre
    // Toda la informacion tecnica (FPS, coordenadas, altitud, escalas) esta
    // oculta por defecto y se alterna con F3.
    bool  modoDebug = false;
    ModoVisualizacion visualizacion = ModoVisualizacion::WireframePuntos;

    void siguienteVisualizacion() {
        int valor = (static_cast<int>(visualizacion) + 1) % 6;
        visualizacion = static_cast<ModoVisualizacion>(valor);
    }
};
