#include "GestorRecursos.h"
#include "UtilidadesGL.h"

#include <iostream>

GLuint GestorRecursos::obtenerShader(const std::string& ruta, GLenum tipo) {
    auto it = shaders.find(ruta);
    if (it != shaders.end()) { ++aciertos; return it->second.obtener(); }

    std::string fuente = leerArchivo(ruta);
    if (fuente.empty()) {
        std::cerr << "[ERROR] Shader inexistente o vacio: " << ruta
                  << ". Comprueba que ejecutas GeoDrone desde la raiz del proyecto.\n";
        huboErrores = true;
        return 0;
    }
    const char* texto = fuente.c_str();

    GLuint id = glCreateShader(tipo);
    glShaderSource(id, 1, &texto, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char registro[512];
        glGetShaderInfoLog(id, 512, nullptr, registro);
        std::cerr << "[ERROR] " << (tipo == GL_VERTEX_SHADER ? "VERT " : "FRAG ") << ruta << ":\n"
                  << registro << "\n";
        glDeleteShader(id);
        huboErrores = true;
        return 0;
    }
    std::cout << "[SHADER] " << ruta << " cargado correctamente\n";
    shaders[ruta] = ShaderGL(id);
    return id;
}

GLuint GestorRecursos::obtenerPrograma(const std::string& rutaVertex, const std::string& rutaFragment) {
    std::string clave = rutaVertex + "|" + rutaFragment;
    auto it = programas.find(clave);
    if (it != programas.end()) { ++aciertos; return it->second.obtener(); }

    GLuint v = obtenerShader(rutaVertex,   GL_VERTEX_SHADER);
    GLuint f = obtenerShader(rutaFragment, GL_FRAGMENT_SHADER);
    if (!v || !f) return 0;

    GLuint programa = glCreateProgram();
    glAttachShader(programa, v);
    glAttachShader(programa, f);
    glLinkProgram(programa);

    int ok = 0;
    glGetProgramiv(programa, GL_LINK_STATUS, &ok);
    if (!ok) {
        char registro[512];
        glGetProgramInfoLog(programa, 512, nullptr, registro);
        std::cerr << "[ERROR] Enlace de programa (" << clave << "):\n" << registro << "\n";
        glDeleteProgram(programa);
        huboErrores = true;
        return 0;
    }
    // Los objetos shader NO se borran aqui: siguen en la cache para que otro
    // programa pueda reutilizarlos sin volver a compilarlos.
    glDetachShader(programa, v);
    glDetachShader(programa, f);

    programas[clave] = ProgramaGL(programa);
    return programa;
}

void GestorRecursos::informarCache() const {
    std::cout << "Recursos: " << programas.size() << " programas, "
              << shaders.size() << " shaders compilados, "
              << aciertos << " reutilizaciones evitadas\n";
}

void GestorRecursos::liberar() {
    // clear() ejecuta los destructores RAII de ProgramaGL y ShaderGL.
    programas.clear();
    shaders.clear();
    aciertos = 0;
    huboErrores = false;
}
