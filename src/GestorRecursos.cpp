#include "GestorRecursos.h"
#include "UtilidadesGL.h"

#include <iostream>

GLuint GestorRecursos::obtenerShader(const std::string& ruta, GLenum tipo) {
    auto it = shaders.find(ruta);
    if (it != shaders.end()) { ++aciertos; return it->second; }

    std::string fuente = leerArchivo(ruta);
    const char* texto = fuente.c_str();

    GLuint id = glCreateShader(tipo);
    glShaderSource(id, 1, &texto, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char registro[512];
        glGetShaderInfoLog(id, 512, nullptr, registro);
        std::cerr << (tipo == GL_VERTEX_SHADER ? "VERT " : "FRAG ") << ruta << ":\n"
                  << registro << "\n";
    }
    shaders[ruta] = id;
    return id;
}

GLuint GestorRecursos::obtenerPrograma(const std::string& rutaVertex, const std::string& rutaFragment) {
    std::string clave = rutaVertex + "|" + rutaFragment;
    auto it = programas.find(clave);
    if (it != programas.end()) { ++aciertos; return it->second; }

    GLuint v = obtenerShader(rutaVertex,   GL_VERTEX_SHADER);
    GLuint f = obtenerShader(rutaFragment, GL_FRAGMENT_SHADER);

    GLuint programa = glCreateProgram();
    glAttachShader(programa, v);
    glAttachShader(programa, f);
    glLinkProgram(programa);

    int ok = 0;
    glGetProgramiv(programa, GL_LINK_STATUS, &ok);
    if (!ok) {
        char registro[512];
        glGetProgramInfoLog(programa, 512, nullptr, registro);
        std::cerr << "LINK (" << clave << "):\n" << registro << "\n";
    }
    // Los objetos shader NO se borran aqui: siguen en la cache para que otro
    // programa pueda reutilizarlos sin volver a compilarlos.
    glDetachShader(programa, v);
    glDetachShader(programa, f);

    programas[clave] = programa;
    return programa;
}

void GestorRecursos::informarCache() const {
    std::cout << "Recursos: " << programas.size() << " programas, "
              << shaders.size() << " shaders compilados, "
              << aciertos << " reutilizaciones evitadas\n";
}

void GestorRecursos::liberar() {
    for (auto& par : programas) glDeleteProgram(par.second);
    for (auto& par : shaders)   glDeleteShader(par.second);
    programas.clear();
    shaders.clear();
    aciertos = 0;
}
