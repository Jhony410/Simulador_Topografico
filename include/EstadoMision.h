#pragma once

// ============================================================================
//  MODELO: estado narrativo de la mision.
//  Guarda cuanto se ha explorado, si la mision ya se completo y el avance del
//  fundido del panel informativo. La Vista solo LEE estos valores; no decide
//  cuando empieza ni cuanto dura la animacion.
// ============================================================================
class EstadoMision {
public:
    void reiniciar();
    void actualizar(float porcentajeExplorado, float dt);

    // 0..1. Cuando la mision se da por completada devuelve exactamente 1.0
    // para que la barra y el texto no se queden en un 99% eterno.
    float obtenerProgreso() const { return completada ? 1.0f : progreso; }
    int   obtenerPorcentaje() const;

    bool  estaCompletada()   const { return completada; }

    // 0..1: opacidad del panel lateral durante su aparicion.
    float obtenerAlphaPanel() const { return alphaPanel; }

private:
    float progreso   = 0.0f;
    bool  completada = false;
    float alphaPanel = 0.0f;
};
