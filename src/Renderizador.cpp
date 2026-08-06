#include "Renderizador.h"
#include "Configuracion.h"
#include "Escena.h"

#include <glad/glad.h>
#include <iostream>

bool Renderizador::inicializar() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    // Mezcla alfa estandar: la rejilla se funde con el fondo segun la distancia.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);   // linea fina: la cuadricula debe ser tenue, no gruesa
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    if (glGetError() != GL_NO_ERROR) {
        glDisable(GL_LINE_SMOOTH);
        std::cout << "[WARN] Line smoothing no disponible; se mantiene MSAA\n";
    }

    vistaTerreno.inicializar(recursos);
    vistaMarcadores.inicializar(recursos);
    vistaMinimapa.inicializar(recursos);
    vistaDron.inicializar(recursos);
    vistaEscaner.inicializar(recursos);
    vistaExploracion.inicializar(recursos);
    vistaCurvas.inicializar(recursos);
    vistaHUD.inicializar(recursos);
    postProceso.inicializar(recursos);

    recursos.informarCache();
    if (recursos.tieneErrores()) {
        std::cerr << "[ERROR] El pipeline grafico no pudo inicializar todos los shaders.\n";
        return false;
    }
    return true;
}

void Renderizador::sincronizarConEscena(Escena& escena) {
    // La malla del dron no cambia entre mapas: se sube una sola vez.
    if (!dronSubido && escena.hayDron()) {
        vistaDron.subirMalla(escena.obtenerMallaDron(), escena.obtenerPivotesDron());
        dronSubido = true;
    }

    bool terrenoCambio = false;
    if (auto* comp = escena.obtenerEntidadTerreno().obtenerComponente<ComponenteMalla>()) {
        if (comp->dirty) {
            vistaTerreno.subirMalla(escena.obtenerTerreno());
            comp->dirty = false;
            terrenoCambio = true;
        }
    }
    // Las estacas se resiembran con el terreno, asi que viajan juntas.
    if (terrenoCambio) {
        vistaMarcadores.subirMarcadores(escena.obtenerMarcadores(),
                                        escena.obtenerTerreno().obtenerLimites());
        vistaMinimapa.subirTerreno(escena.obtenerTerreno());
    }

    // Solo se reconstruye el VBO de curvas cuando el Modelo las regenero.
    if (escena.consumirCurvasSucias())
        vistaCurvas.actualizarCurvas(escena.obtenerCurvas(),
                                     escena.obtenerTerreno().obtenerLimites());
}

void Renderizador::renderizar(Escena& escena, const Camara& camara,
                              int anchoPantalla, int altoPantalla,
                              const EstadoTeclas& teclas, float dt,
                              int opcionMenu, int mapaSeleccionado) {
    metricas.nuevoFrame(dt);
    tiempoTotal += dt;
    vistaEscaner.avanzar(dt);

    float aspecto = (altoPantalla > 0) ? (float)anchoPantalla / (float)altoPantalla : 1.0f;
    const glm::vec3& posicionDron = escena.obtenerDron().obtenerPosicion();
    const EscalaMundo& escala = escena.obtenerEscala();

    // ---- Visibilidad: un solo frustum alimenta a las dos jerarquias ----
    frustum.extraerDe(camara.matrizProyeccion(aspecto) * camara.matrizVista());

    const Quadtree& quadtree = escena.obtenerTerreno().obtenerQuadtree();
    nodosTerrenoVisibles.clear();
    quadtree.seleccionar(frustum, posicionDron, nodosTerrenoVisibles);
    metricas.establecerNodosTerreno((int)nodosTerrenoVisibles.size());

    // Segmentos que se van a enviar frente a los de la malla completa a paso 1.
    int segmentos = 0;
    for (int indice : nodosTerrenoVisibles)
        segmentos += (int)(quadtree.obtenerNodos()[indice].numIndices / 2);
    int lado = escena.obtenerTerreno().obtenerResolucionRejilla();
    metricas.establecerSegmentos(segmentos, 2 * lado * (lado - 1));

    marcadoresVisibles.clear();
    escena.obtenerMarcadores().obtenerBVH().consultar(frustum, marcadoresVisibles);

    // A partir de aqui la escena 3D se dibuja EN TEXTURA, no en pantalla: es lo
    // que permite extraer el brillo y desenfocarlo despues.
    postProceso.iniciarCaptura(anchoPantalla, altoPantalla, Paleta::FONDO);

    // ---- Terreno (rejilla con atenuacion radial) ----
    Entidad& entTerreno = escena.obtenerEntidadTerreno();
    if (auto* material = entTerreno.obtenerComponente<ComponenteMaterial>()) {
        auto* malla = entTerreno.obtenerComponente<ComponenteMalla>();
        // La matriz sale del grafo de escena, no de variables sueltas.
        const glm::mat4& modelo = escena.obtenerNodoTerreno()->obtenerTransformacionMundo();
        vistaTerreno.actualizarExploracion(escena.obtenerMapaExploracion());
        metricas.sumarDrawCalls(
            vistaTerreno.dibujar(camara, modelo, *material,
                                 malla && malla->topologiaLineas, posicionDron, aspecto,
                                 quadtree, nodosTerrenoVisibles, escena.obtenerAjustes(),
                                 escala,
                                 escena.obtenerTerreno().obtenerLimites(),
                                 escena.obtenerTerreno().obtenerLimites().minY,
                                 escena.obtenerTerreno().obtenerLimites().maxY,
                                 tiempoTotal));
    }

    // ---- Marcadores de sondeo ----
    vistaMarcadores.actualizarExploracion(escena.obtenerMapaExploracion());
    metricas.sumarDrawCalls(
        vistaMarcadores.dibujar(camara,
                                escena.obtenerNodoMarcadores()->obtenerTransformacionMundo(),
                                posicionDron, aspecto, escala, marcadoresVisibles));

    metricas.sumarDrawCalls(vistaExploracion.dibujar(
        camara, escena.obtenerTerreno(), escena.obtenerSistemaExploracion(),
        escena.obtenerMapaExploracion(),
        escena.obtenerSistemaMedicion(), escena.obtenerAjustes(), escala, posicionDron,
        aspecto, tiempoTotal));

    // ---- Dron ----
    Entidad& entDron = escena.obtenerEntidadDron();
    auto* materialDron = entDron.obtenerComponente<ComponenteMaterial>();
    auto* animDron     = entDron.obtenerComponente<ComponenteAnimacion>();
    if (materialDron && animDron && escena.hayDron()) {
        const glm::mat4& modelo = escena.obtenerNodoDron()->obtenerTransformacionMundo();
        metricas.sumarDrawCalls(
            vistaDron.dibujar(camara, modelo, *materialDron, *animDron, aspecto));
    }

    // ---- Luz de escaneo: DESPUES del dron y del terreno -------------------
    // Va la ultima de las pasadas opacas porque se mezcla de forma aditiva y
    // sin escribir profundidad: necesita el z-buffer ya resuelto para que el
    // relieve que esta delante del haz lo siga tapando.
    if (auto* escaner = entDron.obtenerComponente<ComponenteEscaner>()) {
        bool escaneando = escaner->activo &&
                          escena.obtenerEstadoAplicacion() == EstadoAplicacion::Playing;
        metricas.sumarDrawCalls(
            vistaEscaner.dibujar(camara, escena.obtenerTerreno(), escala,
                                 posicionDron, aspecto, escaneando));
    }

    // ---- Post-proceso: desenfoca el brillo y compone sobre la pantalla ----
    metricas.sumarDrawCalls(postProceso.componer(anchoPantalla, altoPantalla,
                                                  escena.obtenerAjustes().particulas,
                                                  tiempoTotal));

    metricas.sumarDrawCalls(vistaMinimapa.dibujar(
        escena.obtenerTerreno(), escena.obtenerMapaExploracion(),
        posicionDron, escena.obtenerDron().obtenerYaw(),
        anchoPantalla, altoPantalla, escena.obtenerAjustes().minimapa));

    // ---- HUD 2D ----
    // Va despues de componer: el HUD no debe brillar ni desenfocarse.
    metricas.sumarDrawCalls(
        vistaHUD.dibujar(escena, teclas, (float)anchoPantalla, (float)altoPantalla,
                         metricas, camara, opcionMenu, mapaSeleccionado));
}

void Renderizador::liberar() {
    vistaTerreno.liberar();
    vistaMarcadores.liberar();
    vistaMinimapa.liberar();
    vistaDron.liberar();
    vistaEscaner.liberar();
    vistaExploracion.liberar();
    vistaCurvas.liberar();
    vistaHUD.liberar();
    postProceso.liberar();
    recursos.liberar();   // el gestor es el dueño de todos los programas
}
