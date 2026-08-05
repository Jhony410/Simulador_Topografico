#include "UtilidadesGL.h"

#include <fstream>
#include <iostream>
#include <sstream>

std::string leerArchivo(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cerr << "ERROR: no abre " << ruta << "\n";
        return "";
    }
    std::ostringstream flujo;
    flujo << archivo.rdbuf();
    return flujo.str();
}

namespace {
GLuint compilar(GLenum tipo, const std::string& fuente, const std::string& ruta) {
    const char* texto = fuente.c_str();
    GLuint id = glCreateShader(tipo);
    glShaderSource(id, 1, &texto, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char registro[512];
        glGetShaderInfoLog(id, 512, nullptr, registro);
        std::cerr << (tipo == GL_VERTEX_SHADER ? "VERT " : "FRAG ") << ruta << ":\n" << registro << "\n";
    }
    return id;
}
} // namespace

GLuint crearProgramaShader(const std::string& rutaVertex, const std::string& rutaFragment) {
    GLuint v = compilar(GL_VERTEX_SHADER,   leerArchivo(rutaVertex),   rutaVertex);
    GLuint f = compilar(GL_FRAGMENT_SHADER, leerArchivo(rutaFragment), rutaFragment);

    GLuint programa = glCreateProgram();
    glAttachShader(programa, v);
    glAttachShader(programa, f);
    glLinkProgram(programa);

    int ok = 0;
    glGetProgramiv(programa, GL_LINK_STATUS, &ok);
    if (!ok) {
        char registro[512];
        glGetProgramInfoLog(programa, 512, nullptr, registro);
        std::cerr << "LINK (" << rutaVertex << " + " << rutaFragment << "):\n" << registro << "\n";
    }
    // Ya estan enlazados en el programa: los objetos shader sueltos sobran.
    glDeleteShader(v);
    glDeleteShader(f);
    return programa;
}
