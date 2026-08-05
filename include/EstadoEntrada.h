#pragma once

// ============================================================================
//  Estado de las teclas de vuelo en el frame actual.
//  Es el "puente" entre Controlador y Vista: el Controlador lo llena leyendo
//  GLFW y la VistaHUD lo consulta para resaltar las teclas en pantalla, sin
//  que la Vista tenga que hablar con GLFW.
// ============================================================================
struct EstadoTeclas {
    bool arriba   = false;
    bool abajo    = false;
    bool izquierda = false;
    bool derecha  = false;
    bool espacio  = false;   // subir
    bool shift    = false;   // bajar
};
