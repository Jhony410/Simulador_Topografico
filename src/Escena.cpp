#include "Escena.h"
#include "CargadorModelos.h"
#include "Configuracion.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace fs = std::filesystem;

namespace {
// Diagonal del bounding box de una malla ya normalizada. El cargador deja el
// dron con extension maxima 1.0, asi que este valor cae entre 1.0 y sqrt(3).
float diagonalDe(const MallaCruda& malla) {
    if (malla.vacia() || malla.floatsPorVertice < 3) return 1.0f;
    glm::vec3 minimo(1e9f), maximo(-1e9f);
    for (std::size_t i = 0; i + 2 < malla.vertices.size(); i += malla.floatsPorVertice) {
        glm::vec3 p(malla.vertices[i], malla.vertices[i + 1], malla.vertices[i + 2]);
        minimo = glm::min(minimo, p);
        maximo = glm::max(maximo, p);
    }
    float d = glm::length(maximo - minimo);
    return (d > 1e-4f) ? d : 1.0f;
}
}

Escena::Escena() = default;

bool Escena::inicializar() {
    estadoAplicacion = EstadoAplicacion::Loading;
    std::cout << "[INIT] Descubriendo mapas en assets/\n";
    mapas = CargadorModelos::descubrirTerrenos(Configuracion::CARPETA_ASSETS);
    if (mapas.empty()) {
        mensajeError = "No se encontraron terrenos compatibles en assets/.";
        estadoAplicacion = EstadoAplicacion::Error;
        std::cerr << "[ERROR] " << mensajeError << "\n";
        return false;
    }
    std::cout << "Terrenos:\n";
    for (int i = 0; i < (int)mapas.size(); i++)
        std::cout << "  [" << (i + 1) << "] " << fs::path(mapas[i]).filename().string() << "\n";

    // El dron se carga una sola vez: su malla no depende del mapa activo.
    if (fs::exists(Configuracion::RUTA_DRON)) {
        dronCargado = CargadorModelos::cargarDronAnimado(
            Configuracion::RUTA_DRON, mallaDron, pivotesHelices, Configuracion::TAMANIO_DRON);
    }
    if (!dronCargado) std::cerr << "AVISO: no se cargo el dron\n";
    diagonalMallaDron = diagonalDe(mallaDron);

    construirGrafo();
    construirEntidades();
    if (!cargarMapa(0)) {
        mensajeError = "No fue posible cargar el primer terreno disponible.";
        estadoAplicacion = EstadoAplicacion::Error;
        return false;
    }
    // Se entra DIRECTAMENTE a volar: no hay menu inicial que navegar. La pista
    // "WASD PARA INICIAR LA EXPLORACION" se dibuja unos segundos y se apaga.
    estadoAplicacion = EstadoAplicacion::Playing;
    tiempoDesdeInicio = 0.0f;
    return true;
}

float Escena::obtenerAlphaPistaInicial() const {
    const float visible = Configuracion::DURACION_PISTA_INICIAL;
    const float fade    = Configuracion::FADE_PISTA_INICIAL;
    if (tiempoDesdeInicio >= visible + fade) return 0.0f;
    if (tiempoDesdeInicio <= visible) return 1.0f;
    return 1.0f - (tiempoDesdeInicio - visible) / fade;
}

void Escena::construirGrafo() {
    // raiz -> { terreno, dron -> { helice1..4, sensor }, marcadores, luces }
    nodoTerreno = raiz.agregarHijo(std::make_unique<NodoEscena>("terreno"));
    nodoDron    = raiz.agregarHijo(std::make_unique<NodoEscena>("dron"));

    for (int i = 0; i < 4; i++) {
        nodosHelice[i] = nodoDron->agregarHijo(
            std::make_unique<NodoEscena>("helice" + std::to_string(i + 1)));
        // La local de cada helice es la traslacion a su pivote: asi el nodo ya
        // representa el eje de giro real dentro del espacio del dron.
        nodosHelice[i]->establecerTransformacionLocal(
            glm::translate(glm::mat4(1.0f), pivotesHelices[i]));
    }
    nodoSensor     = nodoDron->agregarHijo(std::make_unique<NodoEscena>("sensor"));
    nodoMarcadores = raiz.agregarHijo(std::make_unique<NodoEscena>("marcadores"));
    nodoLuces      = raiz.agregarHijo(std::make_unique<NodoEscena>("luces"));

    std::cout << "Grafo de escena: " << raiz.contarDescendientes() << " nodos bajo la raiz\n";
}

void Escena::construirEntidades() {
    // ---- Terreno ----
    auto* transTerreno = entidadTerreno.agregarComponente<ComponenteTransformada>();
    transTerreno->nodo = nodoTerreno;

    auto* mallaTerreno = entidadTerreno.agregarComponente<ComponenteMalla>();
    mallaTerreno->malla = &terreno.obtenerMalla();

    auto* matTerreno = entidadTerreno.agregarComponente<ComponenteMaterial>();
    matTerreno->color = Paleta::REJILLA;
    matTerreno->alpha = Configuracion::ALPHA_REJILLA;
    matTerreno->dibujarRelleno = false;

    // ---- Dron ----
    auto* transDron = entidadDron.agregarComponente<ComponenteTransformada>();
    transDron->nodo = nodoDron;

    auto* mallaDronComp = entidadDron.agregarComponente<ComponenteMalla>();
    mallaDronComp->malla = &mallaDron;

    auto* matDron = entidadDron.agregarComponente<ComponenteMaterial>();
    matDron->dibujarRelleno = true;
    matDron->colorRelleno   = glm::vec3(0.10f, 0.09f, 0.02f);  // silueta: tapa el interior
    matDron->alphaRelleno   = 1.0f;
    matDron->color          = Paleta::ACENTO;                  // aristas amarillas
    matDron->alpha          = 1.0f;
    matDron->emision        = 1.0f;                            // es lo unico que genera glow

    auto* animDron = entidadDron.agregarComponente<ComponenteAnimacion>();
    animDron->velocidadGiro = Configuracion::GIRO_HELICES;

    // El radio real lo fija cargarMapa() a partir de EscalaMundo; aqui solo se
    // crea el componente.
    auto* escaner = entidadDron.agregarComponente<ComponenteEscaner>();
    escaner->celdasPorFrame = Configuracion::CELDAS_POR_FRAME;
}

bool Escena::cargarMapa(int indice) {
    if (indice < 0 || indice >= (int)mapas.size()) return false;
    mapaActual = indice;

    const std::string& ruta = mapas[mapaActual];
    std::cout << "\nCargando [" << (mapaActual + 1) << "/" << mapas.size() << "] "
              << fs::path(ruta).filename().string() << " ...\n";

    if (!terreno.cargarDesdeArchivo(ruta)) {
        mensajeError = "Fallo al cargar " + fs::path(ruta).filename().string();
        std::cerr << "[ERROR] " << mensajeError << "\n";
        return false;
    }

    // ---- Escala del mundo: TODO lo demas se deriva de aqui -----------------
    // Se recalcula por mapa, antes de sembrar nada, porque marcadores, escaner,
    // camara y vuelo dependen de ella.
    escala = EscalaMundo::calcular(terreno, diagonalMallaDron);

    // La malla cambio de contenido: la Vista debe volver a subirla a la GPU.
    if (auto* comp = entidadTerreno.obtenerComponente<ComponenteMalla>()) {
        comp->malla = &terreno.obtenerMalla();
        comp->topologiaLineas = terreno.esMallaDeLineas();
        comp->dirty = true;
    }

    // El dron NO cambia de malla entre mapas: cambia su escala en la
    // transformada, para que la proporcion dron/terreno sea la misma en todos.
    if (auto* trans = entidadDron.obtenerComponente<ComponenteTransformada>())
        trans->escala = glm::vec3(escala.escalaDron);
    if (auto* escaner = entidadDron.obtenerComponente<ComponenteEscaner>())
        escaner->radioEscaneo = escala.radioEscaneo;

    // Niebla de guerra limpia para el nuevo mapa. El cambio de mapa reinicia el
    // progreso: la mascara vuelve a cero y con ella el porcentaje.
    mapaExploracion.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla(),
                              terreno.obtenerLimites(), terreno.obtenerCobertura());
    sistemaEscaneo.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla());
    estadoMision.reiniciar();
    generadorCurvas.reiniciar();
    curvas.limpiar();
    curvas.establecerRango(terreno.obtenerLimites().minY, terreno.obtenerLimites().maxY);
    curvasSucias = true;   // el panel debe vaciarse al cambiar de mapa
    misionAnunciada = false;

    // Estacas resembradas con la misma semilla: cada mapa tiene su propio
    // patron, pero siempre el mismo entre ejecuciones.
    marcadores.generar(terreno, escala, Configuracion::NUM_MARCADORES,
                       Configuracion::SEMILLA_MARCADORES);
    sistemaExploracion.generar(terreno, escala, Configuracion::NUM_PUNTOS_ESCANEO,
                               Configuracion::SEMILLA_ESCANEO + static_cast<unsigned int>(indice));
    sistemaMedicion.limpiar();

    // Dron al centro del mapa, a una altura segura sobre el relieve.
    dron.establecerPosicion(glm::vec3(0.0f, terreno.alturaEn(0.0f, 0.0f) + escala.alturaInicial, 0.0f));
    dron.establecerYaw(0.0f);
    if (estadoAplicacion != EstadoAplicacion::Loading)
        estadoAplicacion = EstadoAplicacion::Playing;
    return true;
}

bool Escena::reiniciarMision() {
    return cargarMapa(mapaActual);
}

void Escena::reiniciarEscaneo() {
    if (!terreno.estaCargado()) return;

    // Mascara a cero, con la misma cobertura valida del terreno actual.
    mapaExploracion.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla(),
                              terreno.obtenerLimites(), terreno.obtenerCobertura());
    // La cola del escaner guarda celdas que ya no tienen sentido: se vacia.
    sistemaEscaneo.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla());
    sistemaExploracion.reiniciar();
    estadoMision.reiniciar();

    // Las curvas se derivan de la mascara: hay que rehacerlas desde cero.
    generadorCurvas.reiniciar();
    curvas.limpiar();
    curvasSucias = true;
    misionAnunciada = false;
    std::cout << "[SCAN] Exploracion reiniciada sobre el mismo terreno\n";
}

void Escena::mostrarAviso(const std::string& texto) {
    aviso.mostrar(texto, Configuracion::DURACION_AVISO);
}

int Escena::buscarMapaPorNombre(const std::string& nombreArchivo) const {
    for (int i = 0; i < (int)mapas.size(); ++i)
        if (fs::path(mapas[i]).filename().string() == nombreArchivo) return i;
    return -1;
}

bool Escena::aplicarGuardado(const std::string& nombreMapa,
                             const std::vector<uint32_t>& tiradasRLE,
                             int ancho, int alto,
                             const glm::vec3& posicionDron, float yawDron) {
    int indice = buscarMapaPorNombre(nombreMapa);
    if (indice < 0) return false;

    // cargarMapa deja la mascara a cero y con los limites correctos; solo
    // despues tiene sentido volcar encima la mascara guardada.
    if (indice != mapaActual || !terreno.estaCargado())
        if (!cargarMapa(indice)) return false;

    if (!mapaExploracion.descomprimirRLE(tiradasRLE, ancho, alto)) return false;

    // Todo lo derivado de la mascara queda obsoleto y hay que rehacerlo.
    sistemaEscaneo.reiniciar(terreno.obtenerAnchoGrilla(), terreno.obtenerAnchoGrilla());
    generadorCurvas.reiniciar();
    estadoMision.reiniciar();
    curvas.limpiar();
    curvasSucias = true;
    // Una partida restaurada puede venir ya al 100%: el aviso debe poder
    // volver a dispararse en vez de quedar consumido por la sesion anterior.
    misionAnunciada = false;

    dron.establecerPosicion(posicionDron);
    dron.establecerYaw(yawDron);
    return true;
}

void Escena::actualizar(float dt) {
    aviso.actualizar(dt);
    tiempoDesdeInicio += dt;
    const bool jugando = estadoAplicacion == EstadoAplicacion::Playing;
    if (jugando) dron.actualizar(terreno, escala, dt);

    // Modelo -> componente transformada -> nodo del grafo.
    // La posicion es EXACTAMENTE la del Modelo: no se le suma ninguna
    // flotacion ni oscilacion visual, para que soltar las teclas signifique
    // quedarse quieto de verdad. El giro de las helices vive en el vertex
    // shader y no toca esta transformada.
    if (auto* trans = entidadDron.obtenerComponente<ComponenteTransformada>()) {
        trans->posicion      = dron.obtenerPosicion();
        trans->rotacionEuler = glm::vec3(dron.obtenerPitch(), dron.obtenerYaw(), dron.obtenerRoll());
        trans->volcarAlNodo();
    }
    if (auto* trans = entidadTerreno.obtenerComponente<ComponenteTransformada>())
        trans->volcarAlNodo();

    if (jugando) if (auto* anim = entidadDron.obtenerComponente<ComponenteAnimacion>())
        anim->avanzar(dt);

    // ---- Escaneo: detectar encola, procesarLote desencola un lote acotado ----
    if (jugando) if (auto* escaner = entidadDron.obtenerComponente<ComponenteEscaner>()) {
        if (escaner->activo) {
            const glm::vec3& p = dron.obtenerPosicion();
            sistemaEscaneo.detectar(mapaExploracion, p.x, p.z, escaner->radioEscaneo);
            sistemaEscaneo.procesarLote(mapaExploracion, escaner->celdasPorFrame);
        }
    }
    if (jugando) {
        sistemaExploracion.actualizar(dron.obtenerPosicion(), dron.obtenerRapidez(),
                                      escala, dt, mapaExploracion);
        // El progreso que manda ahora es la COBERTURA REAL del terreno, no los
        // puntos de sondeo: es lo que ve el jugador en el minimapa.
        estadoMision.actualizar(mapaExploracion.porcentajeExplorado(), dt);

        // Completar el mapa no interrumpe el vuelo con una pantalla modal: solo
        // se anuncia una vez y se sigue volando.
        if (!misionAnunciada &&
            mapaExploracion.porcentajeExplorado() >= Configuracion::UMBRAL_MISION_COMPLETA) {
            misionAnunciada = true;
            mostrarAviso("AREA CARTOGRAFIADA AL 100%");
            std::cout << "[MISSION] Exploracion completada en "
                      << sistemaExploracion.obtenerTiempoMision() << " segundos\n";
        }
    }

    // Marching Squares + grafo, con su propio freno de 200 ms.
    if (jugando && ajustes.curvasNivel &&
        generadorCurvas.actualizar(terreno, mapaExploracion, curvas, dt))
        curvasSucias = true;

    // Una sola pasada en preorden actualiza dron, sus 4 helices y el sensor.
    raiz.actualizarTransformaciones(glm::mat4(1.0f));
}
