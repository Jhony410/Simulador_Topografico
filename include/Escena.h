#pragma once
#include "AvisoHUD.h"
#include "Ajustes.h"
#include "Componente.h"
#include "Componentes.h"
#include "CurvasNivel.h"
#include "Dron.h"
#include "EstadoMision.h"
#include "EstadoAplicacion.h"
#include "GeneradorCurvas.h"
#include "MallaCruda.h"
#include "MapaExploracion.h"
#include "MarcadoresSondeo.h"
#include "NodoEscena.h"
#include "SistemaEscaneo.h"
#include "SistemaExploracion.h"
#include "SistemaMedicion.h"
#include "Terreno.h"

#include <memory>
#include <string>
#include <vector>

// ============================================================================
//  MODELO agregado: reune el estado del mundo, el grafo de escena y las
//  entidades con sus componentes. No conoce OpenGL ni GLFW.
//  La Vista lo lee para dibujar; el Controlador lo modifica.
// ============================================================================
class Escena {
public:
    Escena();

    // Construye la jerarquia y las entidades. Se llama una sola vez.
    bool inicializar();

    // Cambia de mapa: recarga el terreno y reposiciona el dron.
    bool cargarMapa(int indice);
    bool reiniciarMision();

    // Indice del mapa cuyo nombre de archivo coincide, o -1 si no esta.
    int buscarMapaPorNombre(const std::string& nombreArchivo) const;

    // Restaura una partida: cambia de mapa si hace falta, descomprime la
    // mascara y recoloca el dron. Devuelve false si el guardado no encaja.
    bool aplicarGuardado(const std::string& nombreMapa,
                         const std::vector<uint32_t>& tiradasRLE,
                         int ancho, int alto,
                         const glm::vec3& posicionDron, float yawDron);

    // Sincroniza entidades -> nodos y propaga las transformaciones en cascada.
    void actualizar(float dt);

    // ---- Modelo del mundo ----
    Terreno&         obtenerTerreno()          { return terreno; }
    const Terreno&   obtenerTerreno()    const { return terreno; }
    Dron&            obtenerDron()             { return dron; }
    const Dron&      obtenerDron()       const { return dron; }
    MapaExploracion& obtenerMapaExploracion()  { return mapaExploracion; }
    CurvasNivel&     obtenerCurvas()           { return curvas; }
    const MarcadoresSondeo& obtenerMarcadores() const { return marcadores; }
    const MapaExploracion&  obtenerMapaExploracion() const { return mapaExploracion; }
    const AvisoHUD&         obtenerAviso() const { return aviso; }
    void mostrarAviso(const std::string& texto);
    const EstadoMision&     obtenerEstadoMision() const { return estadoMision; }
    const SistemaEscaneo&   obtenerSistemaEscaneo() const { return sistemaEscaneo; }
    SistemaExploracion& obtenerSistemaExploracion() { return sistemaExploracion; }
    const SistemaExploracion& obtenerSistemaExploracion() const { return sistemaExploracion; }
    SistemaMedicion& obtenerSistemaMedicion() { return sistemaMedicion; }
    const SistemaMedicion& obtenerSistemaMedicion() const { return sistemaMedicion; }
    Ajustes& obtenerAjustes() { return ajustes; }
    const Ajustes& obtenerAjustes() const { return ajustes; }
    EstadoAplicacion obtenerEstadoAplicacion() const { return estadoAplicacion; }
    void establecerEstadoAplicacion(EstadoAplicacion estado) { estadoAplicacion = estado; }
    const std::string& obtenerError() const { return mensajeError; }

    // La Vista pregunta una vez por frame si debe resubir el VBO de curvas.
    bool consumirCurvasSucias() { bool s = curvasSucias; curvasSucias = false; return s; }

    // ---- Entidades (COP) ----
    Entidad& obtenerEntidadTerreno() { return entidadTerreno; }
    Entidad& obtenerEntidadDron()    { return entidadDron; }

    // ---- Grafo de escena ----
    NodoEscena& obtenerRaiz()          { return raiz; }
    NodoEscena* obtenerNodoTerreno()   { return nodoTerreno; }
    NodoEscena* obtenerNodoDron()      { return nodoDron; }
    NodoEscena* obtenerNodoHelice(int i) { return (i >= 0 && i < 4) ? nodosHelice[i] : nullptr; }
    NodoEscena* obtenerNodoMarcadores() { return nodoMarcadores; }

    // ---- Malla y pivotes del dron (dato, no GPU) ----
    const MallaCruda& obtenerMallaDron()   const { return mallaDron; }
    const glm::vec3*  obtenerPivotesDron() const { return pivotesHelices; }
    bool              hayDron()            const { return dronCargado; }

    // ---- Catalogo de mapas ----
    const std::vector<std::string>& obtenerMapas() const { return mapas; }
    int  obtenerMapaActual() const { return mapaActual; }

private:
    void construirGrafo();
    void construirEntidades();

    // Modelo
    Terreno          terreno;
    Dron             dron;
    MapaExploracion  mapaExploracion;
    CurvasNivel      curvas;
    MarcadoresSondeo marcadores;
    SistemaEscaneo   sistemaEscaneo;
    SistemaExploracion sistemaExploracion;
    SistemaMedicion  sistemaMedicion;
    EstadoMision     estadoMision;
    GeneradorCurvas  generadorCurvas;
    AvisoHUD         aviso;
    bool             curvasSucias = true;
    Ajustes          ajustes;
    EstadoAplicacion estadoAplicacion = EstadoAplicacion::Loading;
    std::string      mensajeError;

    // Geometria del dron: se carga una vez y se reutiliza entre mapas.
    MallaCruda mallaDron;
    glm::vec3  pivotesHelices[4]{};
    bool       dronCargado = false;

    // Grafo de escena
    NodoEscena  raiz{"raiz"};
    NodoEscena* nodoTerreno    = nullptr;
    NodoEscena* nodoDron       = nullptr;
    NodoEscena* nodosHelice[4] = {nullptr, nullptr, nullptr, nullptr};
    NodoEscena* nodoSensor     = nullptr;
    NodoEscena* nodoMarcadores = nullptr;
    NodoEscena* nodoLuces      = nullptr;

    // Entidades con componentes
    Entidad entidadTerreno{"terreno"};
    Entidad entidadDron{"dron"};

    std::vector<std::string> mapas;
    int mapaActual = 0;
};
