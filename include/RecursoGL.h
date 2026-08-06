#pragma once
#include <glad/glad.h>

struct EliminarBuffer { static void ejecutar(GLsizei n,const GLuint* p){ glDeleteBuffers(n,p); } };
struct EliminarVertexArray { static void ejecutar(GLsizei n,const GLuint* p){ glDeleteVertexArrays(n,p); } };
struct EliminarTextura { static void ejecutar(GLsizei n,const GLuint* p){ glDeleteTextures(n,p); } };
struct EliminarFramebuffer { static void ejecutar(GLsizei n,const GLuint* p){ glDeleteFramebuffers(n,p); } };
struct EliminarRenderbuffer { static void ejecutar(GLsizei n,const GLuint* p){ glDeleteRenderbuffers(n,p); } };

template <class Politica>
class RecursoGL {
public:
    RecursoGL() = default;
    explicit RecursoGL(GLuint identificador) : valor(identificador) {}
    ~RecursoGL() { reiniciar(); }

    RecursoGL(const RecursoGL&) = delete;
    RecursoGL& operator=(const RecursoGL&) = delete;
    RecursoGL(RecursoGL&& otro) noexcept : valor(otro.liberarPropiedad()) {}
    RecursoGL& operator=(RecursoGL&& otro) noexcept {
        if (this != &otro) { reiniciar(); valor = otro.liberarPropiedad(); }
        return *this;
    }

    GLuint obtener() const { return valor; }
    explicit operator bool() const { return valor != 0; }
    void adoptar(GLuint nuevo) { reiniciar(); valor = nuevo; }
    void reiniciar() { if (valor) { Politica::ejecutar(1, &valor); valor = 0; } }
    GLuint liberarPropiedad() { GLuint salida = valor; valor = 0; return salida; }

private:
    GLuint valor = 0;
};

using BufferGL = RecursoGL<EliminarBuffer>;
using VertexArrayGL = RecursoGL<EliminarVertexArray>;
using TexturaGL = RecursoGL<EliminarTextura>;
using FramebufferGL = RecursoGL<EliminarFramebuffer>;
using RenderbufferGL = RecursoGL<EliminarRenderbuffer>;

class ShaderGL {
public:
    ShaderGL() = default;
    explicit ShaderGL(GLuint id) : valor(id) {}
    ~ShaderGL() { reiniciar(); }
    ShaderGL(const ShaderGL&) = delete;
    ShaderGL& operator=(const ShaderGL&) = delete;
    ShaderGL(ShaderGL&& otro) noexcept : valor(otro.liberarPropiedad()) {}
    ShaderGL& operator=(ShaderGL&& otro) noexcept { if(this!=&otro){reiniciar();valor=otro.liberarPropiedad();} return *this; }
    GLuint obtener() const { return valor; }
    void reiniciar(){ if(valor){glDeleteShader(valor);valor=0;} }
    GLuint liberarPropiedad(){GLuint r=valor;valor=0;return r;}
private: GLuint valor=0;
};

class ProgramaGL {
public:
    ProgramaGL() = default;
    explicit ProgramaGL(GLuint id) : valor(id) {}
    ~ProgramaGL() { reiniciar(); }
    ProgramaGL(const ProgramaGL&) = delete;
    ProgramaGL& operator=(const ProgramaGL&) = delete;
    ProgramaGL(ProgramaGL&& otro) noexcept : valor(otro.liberarPropiedad()) {}
    ProgramaGL& operator=(ProgramaGL&& otro) noexcept { if(this!=&otro){reiniciar();valor=otro.liberarPropiedad();} return *this; }
    GLuint obtener() const { return valor; }
    void reiniciar(){ if(valor){glDeleteProgram(valor);valor=0;} }
    GLuint liberarPropiedad(){GLuint r=valor;valor=0;return r;}
private: GLuint valor=0;
};
