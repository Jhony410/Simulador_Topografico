#pragma once

// ============================================================================
//  VISTA: metricas de render para el HUD.
//  Los FPS se promedian en ventanas de medio segundo: el inverso del delta de
//  un frame suelto salta tanto que el numero seria ilegible.
// ============================================================================
class ContadorRendimiento {
public:
    // Se llama una vez por frame, antes de dibujar.
    void nuevoFrame(float dt);

    // Cada Vista suma las llamadas de dibujo que ha emitido.
    void sumarDrawCalls(int cantidad) { drawCallsFrame += cantidad; }
    void reiniciarDrawCalls() { drawCallsFrame = 0; }

    float obtenerFPS()        const { return fps; }
    int   obtenerDrawCalls()  const { return drawCallsUltimoFrame; }
    int   obtenerNodosTerreno() const { return nodosTerreno; }
    void  establecerNodosTerreno(int n) { nodosTerreno = n; }

    // Segmentos de rejilla realmente enviados frente a los que tendria la malla
    // completa: es la cifra que demuestra lo que ahorra el nivel de detalle.
    int   obtenerSegmentosDibujados() const { return segmentosDibujados; }
    int   obtenerSegmentosTotales()   const { return segmentosTotales; }
    void  establecerSegmentos(int dibujados, int totales) {
        segmentosDibujados = dibujados;
        segmentosTotales = totales;
    }

private:
    float acumulador = 0.0f;
    int   framesAcumulados = 0;
    float fps = 0.0f;

    int drawCallsFrame = 0;
    int drawCallsUltimoFrame = 0;
    int nodosTerreno = 0;
    int segmentosDibujados = 0;
    int segmentosTotales = 0;
};
