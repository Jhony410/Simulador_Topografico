#pragma once
#include <glad/glad.h>
#include <string>

// ============================================================================
//  VISTA: utilidades de bajo nivel de OpenGL compartidas por los renderers.
// ============================================================================

// Lee un archivo de texto completo (shaders). Devuelve "" si no existe.
std::string leerArchivo(const std::string& ruta);

// Compila vertex + fragment y enlaza el programa. Reporta errores por consola.
GLuint crearProgramaShader(const std::string& rutaVertex, const std::string& rutaFragment);
