#pragma once
#include <glad/glad.h>
#include "RecursoGL.h"
#include <string>
#include <unordered_map>

// ============================================================================
//  VISTA: cache de recursos de GPU.
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS
//  ---------------------------------------
//  Dos std::unordered_map con la RUTA DEL ARCHIVO como clave:
//      shaders   : ruta            -> objeto shader ya compilado
//      programas : "vert|frag"     -> programa ya enlazado
//
//  La clave natural es una cadena, no un numero, y la operacion dominante es
//  "¿ya compile esto?" repetida en cada arranque de vista. El unordered_map
//  responde en O(1) promedio; un std::map ordenado costaria O(log n) y no
//  aporta nada porque el orden alfabetico de las rutas es irrelevante, y un
//  vector de pares obligaria a comparar cadenas una por una.
//
//  El beneficio no es teorico: 'composicion.vert' lo comparten los tres
//  programas de post-proceso (brillo, desenfoque y composicion). Sin cache se
//  compilaria tres veces; con ella, una. Al arrancar se imprime cuantas
//  compilaciones se ahorraron.
// ============================================================================
class GestorRecursos {
public:
    GestorRecursos() = default;
    GestorRecursos(const GestorRecursos&) = delete;
    GestorRecursos& operator=(const GestorRecursos&) = delete;
    // Devuelve el programa enlazado, compilando solo lo que aun no este en cache.
    GLuint obtenerPrograma(const std::string& rutaVertex, const std::string& rutaFragment);

    void liberar();
    void informarCache() const;

    std::size_t programasEnCache() const { return programas.size(); }
    std::size_t shadersEnCache()   const { return shaders.size(); }
    int reutilizaciones()          const { return aciertos; }
    bool tieneErrores() const { return huboErrores; }

private:
    GLuint obtenerShader(const std::string& ruta, GLenum tipo);

    std::unordered_map<std::string, ShaderGL> shaders;
    std::unordered_map<std::string, ProgramaGL> programas;
    int aciertos = 0;   // cuantas veces se evito recompilar o reenlazar
    bool huboErrores = false;
};
