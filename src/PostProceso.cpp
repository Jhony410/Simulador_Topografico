#include "PostProceso.h"
#include "Configuracion.h"
#include "GestorRecursos.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {
// Quad de pantalla completa en coordenadas de recorte: [x, y, u, v].
const float VERTICES_QUAD[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f
};
} // namespace

bool PostProceso::inicializar(GestorRecursos& recursos) {
    progBrillo      = recursos.obtenerPrograma("shaders/composicion.vert", "shaders/brillo.frag");
    progDesenfoque  = recursos.obtenerPrograma("shaders/composicion.vert", "shaders/desenfoque.frag");
    progComposicion = recursos.obtenerPrograma("shaders/composicion.vert", "shaders/composicion.frag");

    glGenVertexArrays(1, &vaoQuad);
    glGenBuffers(1, &vboQuad);
    glBindVertexArray(vaoQuad);
    glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES_QUAD), VERTICES_QUAD, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Las unidades de textura se asignan una sola vez: no cambian nunca.
    glUseProgram(progBrillo);
    glUniform1i(glGetUniformLocation(progBrillo, "uBrillo"), 0);
    glUseProgram(progDesenfoque);
    glUniform1i(glGetUniformLocation(progDesenfoque, "uTextura"), 0);
    glUseProgram(progComposicion);
    glUniform1i(glGetUniformLocation(progComposicion, "uColor"),  0);
    glUniform1i(glGetUniformLocation(progComposicion, "uBrillo"), 1);
    glUseProgram(0);

    return true;
}

void PostProceso::destruirObjetivos() {
    if (fboEscena)      { glDeleteFramebuffers(1, &fboEscena); fboEscena = 0; }
    if (texColor)       { glDeleteTextures(1, &texColor); texColor = 0; }
    if (texBrillo)      { glDeleteTextures(1, &texBrillo); texBrillo = 0; }
    if (rboProfundidad) { glDeleteRenderbuffers(1, &rboProfundidad); rboProfundidad = 0; }
    if (fboMsaa) { glDeleteFramebuffers(1, &fboMsaa); fboMsaa = 0; }
    if (rboColorMsaa[0]) { glDeleteRenderbuffers(2, rboColorMsaa); rboColorMsaa[0] = rboColorMsaa[1] = 0; }
    if (rboProfundidadMsaa) { glDeleteRenderbuffers(1, &rboProfundidadMsaa); rboProfundidadMsaa = 0; }
    for (int i = 0; i < 2; ++i) {
        if (fboPing[i]) { glDeleteFramebuffers(1, &fboPing[i]); fboPing[i] = 0; }
        if (texPing[i]) { glDeleteTextures(1, &texPing[i]); texPing[i] = 0; }
    }
    listo = false;
    usaMsaa = false;
}

void PostProceso::crearObjetivos(int nuevoAncho, int nuevoAlto) {
    destruirObjetivos();
    ancho = std::max(1, nuevoAncho);
    alto  = std::max(1, nuevoAlto);
    anchoBrillo = std::max(1, ancho / Configuracion::DIVISOR_RESOLUCION_BRILLO);
    altoBrillo  = std::max(1, alto  / Configuracion::DIVISOR_RESOLUCION_BRILLO);

    auto crearTextura = [](GLuint& tex, int w, int h) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        // RGBA16F: el brillo puede pasar de 1.0 sin recortarse, que es lo que
        // hace que el halo tenga rango en vez de saturar de golpe.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // CLAMP_TO_EDGE evita que el desenfoque arrastre pixeles del lado
        // opuesto de la pantalla al muestrear fuera del borde.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };

    // ---- FBO de escena con dos attachments de color ----
    glGenFramebuffers(1, &fboEscena);
    glBindFramebuffer(GL_FRAMEBUFFER, fboEscena);
    crearTextura(texColor,  ancho, alto);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColor, 0);
    crearTextura(texBrillo, ancho, alto);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texBrillo, 0);

    glGenRenderbuffers(1, &rboProfundidad);
    glBindRenderbuffer(GL_RENDERBUFFER, rboProfundidad);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ancho, alto);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboProfundidad);

    const GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: FBO de escena incompleto\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // ---- Par de FBOs para el ping-pong del desenfoque ----
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &fboPing[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, fboPing[i]);
        crearTextura(texPing[i], anchoBrillo, altoBrillo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texPing[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: FBO de desenfoque " << i << " incompleto\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---- FBO multisample: la escena se rasteriza aqui y luego se resuelve a
    // las dos texturas anteriores. Asi MSAA tambien funciona con postproceso.
    glGenFramebuffers(1, &fboMsaa);
    glBindFramebuffer(GL_FRAMEBUFFER, fboMsaa);
    glGenRenderbuffers(2, rboColorMsaa);
    for (int i = 0; i < 2; ++i) {
        glBindRenderbuffer(GL_RENDERBUFFER, rboColorMsaa[i]);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, muestrasMsaa, GL_RGBA16F, ancho, alto);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                  GL_RENDERBUFFER, rboColorMsaa[i]);
    }
    glGenRenderbuffers(1, &rboProfundidadMsaa);
    glBindRenderbuffer(GL_RENDERBUFFER, rboProfundidadMsaa);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, muestrasMsaa, GL_DEPTH_COMPONENT24, ancho, alto);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboProfundidadMsaa);
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[WARN] FBO MSAA incompleto; se usara renderizado sin multisample\n";
        glDeleteFramebuffers(1, &fboMsaa); fboMsaa = 0;
        glDeleteRenderbuffers(2, rboColorMsaa); rboColorMsaa[0] = rboColorMsaa[1] = 0;
        glDeleteRenderbuffers(1, &rboProfundidadMsaa); rboProfundidadMsaa = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        listo = true;
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    usaMsaa = true;
    listo = true;
}

void PostProceso::iniciarCaptura(int nuevoAncho, int nuevoAlto, const glm::vec3& colorFondo) {
    if (nuevoAncho <= 0 || nuevoAlto <= 0) return;
    if (nuevoAncho != ancho || nuevoAlto != alto || !listo)
        crearObjetivos(nuevoAncho, nuevoAlto);
    if (!listo) return;

    glBindFramebuffer(GL_FRAMEBUFFER, usaMsaa ? fboMsaa : fboEscena);
    glViewport(0, 0, ancho, alto);

    // Cada attachment se limpia por separado: el 0 al color de fondo y el 1 a
    // negro transparente, para que el brillo arranque vacio cada frame.
    const GLfloat fondo[4] = {colorFondo.r, colorFondo.g, colorFondo.b, 1.0f};
    const GLfloat negro[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, fondo);
    glClearBufferfv(GL_COLOR, 1, negro);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void PostProceso::dibujarQuad() {
    glBindVertexArray(vaoQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

int PostProceso::componer(int anchoPantalla, int altoPantalla, bool particulas, float tiempo) {
    if (!listo) return 0;

    // Resolver por separado color y emision desde el framebuffer 4x MSAA a
    // texturas normales, que son las que los shaders pueden muestrear.
    if (usaMsaa) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMsaa);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboEscena);
        for (int i = 0; i < 2; ++i) {
            glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
            glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
            glBlitFramebuffer(0, 0, ancho, alto, 0, 0, ancho, alto,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }

    // El post-proceso trabaja pixel a pixel sobre imagenes ya terminadas: ni
    // profundidad ni mezcla tienen sentido aqui.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // ---- 1) Extraccion de brillo a media resolucion ----
    glBindFramebuffer(GL_FRAMEBUFFER, fboPing[0]);
    glViewport(0, 0, anchoBrillo, altoBrillo);
    glUseProgram(progBrillo);
    glUniform1f(glGetUniformLocation(progBrillo, "uUmbral"), Configuracion::UMBRAL_BRILLO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texBrillo);
    dibujarQuad();

    // ---- 2) Desenfoque separable en ping-pong ----
    glUseProgram(progDesenfoque);
    GLint locDireccion = glGetUniformLocation(progDesenfoque, "uDireccion");
    bool horizontal = true;
    int origen = 0;   // texPing[0] ya tiene el brillo extraido
    for (int pasada = 0; pasada < Configuracion::PASADAS_DESENFOQUE; ++pasada) {
        int destino = 1 - origen;
        glBindFramebuffer(GL_FRAMEBUFFER, fboPing[destino]);
        glUniform2f(locDireccion,
                    horizontal ? 1.0f / (float)anchoBrillo : 0.0f,
                    horizontal ? 0.0f : 1.0f / (float)altoBrillo);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texPing[origen]);
        dibujarQuad();

        origen = destino;
        horizontal = !horizontal;
    }

    // ---- 3) Composicion aditiva sobre el framebuffer por defecto ----
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, anchoPantalla, altoPantalla);
    glUseProgram(progComposicion);
    glUniform1f(glGetUniformLocation(progComposicion, "uIntensidadGlow"),
                Configuracion::INTENSIDAD_GLOW);
    glUniform1i(glGetUniformLocation(progComposicion, "uParticulas"), particulas ? 1 : 0);
    glUniform1f(glGetUniformLocation(progComposicion, "uTiempo"), tiempo);
    glUniform3fv(glGetUniformLocation(progComposicion, "uFondoBajo"), 1, glm::value_ptr(Paleta::FONDO));
    glUniform3fv(glGetUniformLocation(progComposicion, "uFondoAlto"), 1, glm::value_ptr(Paleta::FONDO_ALTO));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texPing[origen]);
    dibujarQuad();

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // 1 extraccion + N desenfoques + 1 composicion.
    return 2 + Configuracion::PASADAS_DESENFOQUE;
}

void PostProceso::liberar() {
    destruirObjetivos();
    if (vaoQuad) { glDeleteVertexArrays(1, &vaoQuad); glDeleteBuffers(1, &vboQuad); }
    // Los programas los posee el GestorRecursos.
    vaoQuad = vboQuad = progBrillo = progDesenfoque = progComposicion = 0;
}
