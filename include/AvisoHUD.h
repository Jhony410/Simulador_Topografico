#pragma once
#include <string>

// ============================================================================
//  MODELO: aviso temporal del HUD (guardado / cargado / error).
//  Vive en el Modelo y no en la Vista porque quien lo dispara es el
//  Controlador, y la Vista solo debe LEER que hay que mostrar y con que
//  opacidad. Asi el mensaje sobrevive aunque se reconstruya la capa grafica.
// ============================================================================
class AvisoHUD {
public:
    void mostrar(std::string nuevoTexto, float duracion);
    void actualizar(float dt);
    void ocultar();

    bool visible() const { return restante > 0.0f; }
    const std::string& obtenerTexto() const { return texto; }

    // 1.0 mientras dura y se desvanece en el ultimo tramo.
    float obtenerAlpha() const;

private:
    std::string texto;
    float restante = 0.0f;
    float duracionTotal = 1.0f;
};
