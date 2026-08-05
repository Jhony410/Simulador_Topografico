#pragma once
#include <string>

class Escena;

// ============================================================================
//  MODELO: guardar y recuperar la partida en un archivo JSON.
//  No toca OpenGL: solo lee y escribe estado del Modelo. La Vista se entera
//  sola porque la Escena marca sus datos como sucios al restaurar.
// ============================================================================
namespace Persistencia {

// Devuelven false y dejan un motivo en 'mensaje' si algo falla; el llamador lo
// muestra tal cual en el HUD.
bool guardar(const Escena& escena, const std::string& ruta, std::string& mensaje);
bool cargar(Escena& escena, const std::string& ruta, std::string& mensaje);

} // namespace Persistencia
