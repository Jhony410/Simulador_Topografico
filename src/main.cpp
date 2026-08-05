// ============================================================================
//  Simulador Topografico con Dron  -  FASE 1 (arquitectura MVC)
//  OpenGL 3.3 Core Profile + C++17
//
//  ARQUITECTURA
//    MODELO      (sin OpenGL) : Escena, Terreno, Dron, MapaExploracion,
//                               CurvasNivel, CargadorModelos, NodoEscena,
//                               Entidad/Componentes.
//    VISTA       (unica capa que habla con la GPU) : Renderizador, VistaTerreno,
//                               VistaDron, VistaCurvas, VistaHUD, Camara.
//    CONTROLADOR (traduce input): ControladorEntrada.
//
//  CONTROLES
//    Flechas / WASD  : mover el dron
//    Espacio / Shift : subir / bajar
//    Arrastrar mouse : rotar vista       Scroll: zoom
//    Botones 1..N (clic) o teclas 1..9 : cambiar de mapa
//    TAB / BACKSPACE : mapa siguiente / anterior
//    ESC             : salir
// ============================================================================

#include "Aplicacion.h"

int main() {
    Aplicacion aplicacion;
    if (!aplicacion.inicializar()) return -1;
    aplicacion.ejecutar();
    aplicacion.liberar();
    return 0;
}
