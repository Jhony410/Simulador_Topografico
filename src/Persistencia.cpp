#include "Persistencia.h"
#include "Escena.h"

// json.hpp es un encabezado enorme: se incluye SOLO en esta unidad para no
// cargar su coste de compilacion sobre el resto del proyecto.
#include "json.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace {
constexpr int VERSION_FORMATO = 1;
}

namespace Persistencia {

bool guardar(const Escena& escena, const std::string& ruta, std::string& mensaje) {
    const MapaExploracion& mapa = escena.obtenerMapaExploracion();
    const Dron& dron = escena.obtenerDron();

    if (mapa.obtenerAncho() <= 0) {
        mensaje = "NADA QUE GUARDAR";
        return false;
    }

    json raiz;
    raiz["version"] = VERSION_FORMATO;
    raiz["mapa"] = escena.obtenerTerreno().obtenerNombreArchivo();
    raiz["porcentajeExplorado"] = mapa.porcentajeExplorado();

    raiz["mascara"] = {
        {"ancho", mapa.obtenerAncho()},
        {"alto",  mapa.obtenerAlto()},
        {"rle",   mapa.comprimirRLE()}
    };

    const glm::vec3& p = dron.obtenerPosicion();
    raiz["dron"] = {
        {"x", p.x}, {"y", p.y}, {"z", p.z}, {"yaw", dron.obtenerYaw()}
    };

    std::ofstream archivo(ruta);
    if (!archivo.is_open()) {
        mensaje = "NO SE PUDO ESCRIBIR " + ruta;
        return false;
    }
    archivo << raiz.dump(2);
    if (!archivo.good()) {
        mensaje = "ERROR AL ESCRIBIR " + ruta;
        return false;
    }

    mensaje = "PARTIDA GUARDADA";
    std::cout << "Guardado en " << ruta << " ("
              << mapa.comprimirRLE().size() << " tiradas RLE para "
              << (mapa.obtenerAncho() * mapa.obtenerAlto()) << " celdas)\n";
    return true;
}

bool cargar(Escena& escena, const std::string& ruta, std::string& mensaje) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        mensaje = "NO HAY PARTIDA GUARDADA";
        return false;
    }

    json raiz;
    try {
        archivo >> raiz;
    } catch (const std::exception& e) {
        // El archivo lo puede haber tocado un humano: no se puede confiar en el.
        std::cerr << "ERROR json: " << e.what() << "\n";
        mensaje = "ARCHIVO DE GUARDADO CORRUPTO";
        return false;
    }

    if (!raiz.contains("version") || raiz["version"].get<int>() != VERSION_FORMATO) {
        mensaje = "VERSION DE GUARDADO INCOMPATIBLE";
        return false;
    }
    if (!raiz.contains("mapa") || !raiz.contains("mascara") || !raiz.contains("dron")) {
        mensaje = "ARCHIVO DE GUARDADO INCOMPLETO";
        return false;
    }

    std::string nombreMapa = raiz["mapa"].get<std::string>();
    const json& jm = raiz["mascara"];
    int ancho = jm.value("ancho", 0);
    int alto  = jm.value("alto", 0);
    std::vector<uint32_t> tiradas = jm.value("rle", std::vector<uint32_t>{});

    const json& jd = raiz["dron"];
    glm::vec3 posicion(jd.value("x", 0.0f), jd.value("y", 0.0f), jd.value("z", 0.0f));
    float yaw = jd.value("yaw", 0.0f);

    if (!escena.aplicarGuardado(nombreMapa, tiradas, ancho, alto, posicion, yaw)) {
        mensaje = "EL GUARDADO NO ENCAJA CON ESTE MAPA";
        return false;
    }

    // El porcentaje guardado es redundante (sale de la mascara), pero sirve de
    // comprobacion cruzada: si no cuadra, el archivo esta mal formado.
    float esperado = raiz.value("porcentajeExplorado", -1.0f);
    float real = escena.obtenerMapaExploracion().porcentajeExplorado();
    if (esperado >= 0.0f && std::abs(esperado - real) > 0.005f)
        std::cerr << "AVISO: el porcentaje guardado (" << esperado
                  << ") no coincide con la mascara (" << real << ")\n";

    mensaje = "PARTIDA CARGADA";
    return true;
}

} // namespace Persistencia
