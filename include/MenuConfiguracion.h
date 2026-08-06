#pragma once

// ============================================================================
//  Definicion del menu de configuracion (ESC).
//
//  Vive aparte porque lo comparten DOS capas: el Controlador, que mueve la
//  seleccion y ejecuta la fila elegida, y la Vista, que la dibuja. Tener los
//  indices y las etiquetas en un solo sitio evita que se desincronicen (que la
//  fila resaltada y la accion ejecutada dejen de coincidir).
// ============================================================================

enum FilaMenu {
    FILA_CONTINUAR = 0,
    FILA_MAPA,          // fila con valor: flechas izquierda/derecha eligen terreno
    FILA_REINICIAR,
    FILA_CONTROLES,     // fila con valor: alterna la ayuda en pantalla
    FILA_SALIR,
    OPCIONES_MENU       // debe quedar la ultima: es el numero de filas
};

inline const char* etiquetaFilaMenu(int fila) {
    switch (fila) {
        case FILA_CONTINUAR: return "CONTINUAR";
        case FILA_MAPA:      return "TERRENO";
        case FILA_REINICIAR: return "REINICIAR ESCANEO";
        case FILA_CONTROLES: return "AYUDA EN PANTALLA";
        case FILA_SALIR:     return "SALIR";
    }
    return "";
}

// Las filas con valor muestran flechas y responden a izquierda/derecha.
inline bool filaTieneValor(int fila) {
    return fila == FILA_MAPA || fila == FILA_CONTROLES;
}
