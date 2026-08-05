#pragma once
#include <glm/glm.hpp>
#include <string>

// ============================================================================
//  Configuracion global del simulador.
//  Se agrupa aqui para que ajustar el "feeling" (velocidades, escalas, camara)
//  no obligue a abrir el Modelo, la Vista ni el Controlador.
// ============================================================================
namespace Configuracion {

// ---- Terreno ----
inline constexpr float ANCHO_OBJETIVO     = 100.0f;  // ancho al que se normaliza cualquier mapa
inline constexpr float EXAGERACION_Y      = 1.0f;    // multiplicador del relieve
inline constexpr int   PUNTOS_MAX         = 35000;   // tope de la nube de puntos diluida
inline constexpr int   RESOLUCION_GRILLA  = 256;     // lado del mapa de alturas
// 129 VERTICES = 128 celdas = 2^7, divisible entre 2^5: es lo que permite que
// el Quadtree de profundidad 5 corte siempre sobre vertices existentes.
inline constexpr int   RESOLUCION_REJILLA = 129;

// ---- Atenuacion radial (look Orano: nitido bajo el dron, negro a lo lejos) --
// Referencia: el terreno mide ANCHO_OBJETIVO (100) de lado, asi que radios por
// encima de ~70 dejarian el mapa entero dentro de la zona nitida y la caida no
// se veria. Estos valores hacen que el borde del mapa ya este casi apagado.
inline constexpr float RADIO_NITIDO            = 18.0f;
inline constexpr float RADIO_DESVANECIDO       = 72.0f;
inline constexpr float ALPHA_REJILLA           = 1.0f;
// Las estacas aguantan mas lejos que la rejilla, si no el mapa se ve vacio.
inline constexpr float RADIO_NITIDO_MARCADOR      = 30.0f;
inline constexpr float RADIO_DESVANECIDO_MARCADOR = 110.0f;
inline constexpr float ALPHA_MARCADORES        = 0.95f;

// ---- Marcadores de sondeo ----
inline constexpr int          NUM_MARCADORES        = 120;
inline constexpr unsigned int SEMILLA_MARCADORES    = 20261u;  // fija = escena reproducible
inline constexpr float        MARCADOR_ALTURA_MIN   = 2.5f;
inline constexpr float        MARCADOR_ALTURA_MAX   = 6.5f;
inline constexpr float        MARCADOR_TAM_CABEZA   = 0.42f;   // lado del cuadrito, en mundo
inline constexpr float        MARCADOR_PROB_FLOTANTE = 0.35f;
inline constexpr float        MARCADOR_FLOTE_MAX    = 22.0f;

// ---- Dron ----
// Mas pequeno que antes a proposito: encoge el dron respecto al terreno y
// hace que el relieve se lea mas grande en pantalla.
inline constexpr float TAMANIO_DRON       = 5.0f;
// Umbral del angulo diedro para considerar que una arista es un canto real.
// Por debajo de ~20 grados empiezan a colarse aristas de superficie lisa.
inline constexpr float ANGULO_ARISTA_DRON = 26.0f;
inline constexpr float VEL_DRON           = 50.0f;
inline constexpr float VEL_ALTURA         = 35.0f;
inline constexpr float ALTURA_INICIAL     = 18.0f;
inline constexpr float GIRO_HELICES       = 18.0f;   // rad/seg
inline constexpr float OFFSET_YAW_DRON    = 0.0f;    // correccion si el modelo mira mal

// ---- Camara orbital ----
inline constexpr float CAM_YAW_INICIAL    = 45.0f;
// Angulo bajo, casi a ras del relieve: es lo que da la lectura de "vuelo" en
// vez de la de mapa visto desde arriba.
inline constexpr float CAM_ELEV_INICIAL   = 14.0f;
inline constexpr float CAM_RADIO_INICIAL  = 46.0f;
// Fraccion de distancia que aun quedaria por recorrer al cabo de 1 segundo.
inline constexpr float CAM_BASE_SUAVIZADO = 0.001f;
inline constexpr float CAM_ELEV_MIN       = 8.0f;
inline constexpr float CAM_ELEV_MAX       = 80.0f;
inline constexpr float CAM_RADIO_MIN      = 22.0f;
inline constexpr float CAM_RADIO_MAX      = 260.0f;
inline constexpr float CAM_FOV            = 45.0f;
inline constexpr float CAM_CERCANO        = 0.1f;
inline constexpr float CAM_LEJANO         = 1500.0f;

// ---- Ventana ----
inline constexpr int   ANCHO_VENTANA      = 1280;
inline constexpr int   ALTO_VENTANA       = 720;

// ---- Escaneo / niebla de guerra ----
inline constexpr float RADIO_ESCANEO           = 14.0f;  // unidades de mundo
inline constexpr int   CELDAS_POR_FRAME        = 256;    // lote que desencola el sistema
inline constexpr float UMBRAL_MISION_COMPLETA  = 0.995f;
inline constexpr float DURACION_FADE_PANEL     = 1.2f;   // segundos

// ---- Curvas de nivel ----
inline constexpr int   NIVELES_CURVAS    = 12;
inline constexpr int   PASO_CURVAS       = 2;     // submuestreo de la grilla
inline constexpr float INTERVALO_CURVAS  = 0.2f;  // regeneracion como mucho cada 200 ms

// ---- Quadtree LOD + culling ----
inline constexpr int   PROFUNDIDAD_QUADTREE = 5;
// Un cuadrante deja de subdividirse cuando su distancia al dron supera este
// factor por su propio lado. Mas alto = mas detalle lejos y mas draw calls.
inline constexpr float FACTOR_LOD           = 1.6f;
inline constexpr float INTERVALO_FPS        = 0.5f;   // ventana de promediado

// ---- Glow / post-proceso ----
inline constexpr int   PASADAS_DESENFOQUE          = 8;     // 4 horizontales + 4 verticales
inline constexpr int   DIVISOR_RESOLUCION_BRILLO   = 2;     // el halo se desenfoca a media res
inline constexpr float UMBRAL_BRILLO               = 0.20f;
inline constexpr float INTENSIDAD_GLOW             = 0.95f;

// ---- Textos fijos del HUD (sin tildes: stb_easy_font solo cubre ASCII) ----
inline const std::string TITULO_APP    = "SIMULADOR TOPOGRAFICO";
inline const std::string PIE_PROYECTO  = "UANCV / COMPUTACION GRAFICA SIS226";

// ---- Persistencia ----
inline constexpr float DURACION_AVISO = 2.6f;   // segundos que dura el mensaje

// ---- Rutas ----
inline const std::string RUTA_DRON        = "assets/animated_drone.glb";
inline const std::string CARPETA_ASSETS   = "assets";
inline const std::string RUTA_GUARDADO    = "guardado.json";

} // namespace Configuracion

// ============================================================================
//  Paleta del simulador (referencia Orano).
// ============================================================================
namespace Paleta {
inline const glm::vec3 FONDO        {0.020f, 0.027f, 0.051f};  // #05070d
inline const glm::vec3 REJILLA      {0.749f, 0.820f, 0.949f};  // #BFD1F2
inline const glm::vec3 ACENTO       {1.000f, 0.898f, 0.000f};  // #FFE500
inline const glm::vec3 TEXTO_SEC    {0.533f, 0.573f, 0.651f};  // #8892A6
inline const glm::vec3 BLANCO       {1.000f, 1.000f, 1.000f};
} // namespace Paleta
