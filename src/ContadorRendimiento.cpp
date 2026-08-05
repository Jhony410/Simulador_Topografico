#include "ContadorRendimiento.h"
#include "Configuracion.h"

void ContadorRendimiento::nuevoFrame(float dt) {
    // El recuento del frame anterior ya esta cerrado: se guarda para mostrarlo
    // y se pone a cero el del frame que empieza.
    drawCallsUltimoFrame = drawCallsFrame;
    drawCallsFrame = 0;

    acumulador += dt;
    ++framesAcumulados;
    if (acumulador >= Configuracion::INTERVALO_FPS) {
        fps = framesAcumulados / acumulador;
        acumulador = 0.0f;
        framesAcumulados = 0;
    }
}
