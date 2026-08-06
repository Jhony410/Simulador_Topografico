#pragma once
#include "Limites.h"

#include <cstdint>
#include <vector>

// ============================================================================
//  MODELO: niebla de guerra / mascara de exploracion.
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS
//  ---------------------------------------
//  std::vector<uint8_t> aplanado con acceso [z * ancho + x], mismo tamaño que
//  la grilla del heightmap. Se elige uint8_t y no bool ni un set de celdas:
//   - std::vector<bool> esta especializado en bits: el acceso exige mascaras y
//     no se puede tomar la direccion de un elemento; el ahorro de memoria no
//     compensa (256x256 = 64 KB, despreciable).
//   - Un std::set<pair<int,int>> daria O(log n) por consulta y mucha memoria
//     por nodo; aqui la consulta es "¿esta marcada esta celda?" millones de
//     veces por segundo, y el arreglo la resuelve en O(1) con indice directo.
//
//  Se mantiene un contador incremental de celdas marcadas para que
//  porcentajeExplorado() sea O(1) y no O(n) por frame.
// ============================================================================
class MapaExploracion {
public:
    // Valor que se escribe en una celda explorada.
    //
    // Es 255 y no 1 A PROPOSITO: esta misma mascara se sube tal cual como
    // textura GL_R8, que es un formato NORMALIZADO. Un byte 1 llegaria al
    // shader como 1/255 = 0.004 -practicamente cero- y el revelado del
    // minimapa nunca se veria. Con 255 el shader lee exactamente 1.0.
    // El resto del codigo solo comprueba "distinto de cero", asi que el valor
    // concreto no cambia ninguna otra logica.
    static constexpr uint8_t MARCADA = 255;

    // Prepara la mascara vacia para una grilla y una caja de mundo dadas.
    // 'validez' es la cobertura real del terreno (1 = la celda existe): solo
    // esas celdas entran en el denominador del porcentaje. Si se pasa vacia se
    // considera valida toda la grilla.
    void reiniciar(int ancho, int alto, const LimitesMundo& limites,
                   const std::vector<uint8_t>& validez = {});

    // Marca como exploradas las celdas dentro de un radio en coordenadas de mundo.
    void marcarZona(float x, float z, float radio);

    // Marca una celda concreta. Devuelve true si cambio de estado.
    bool marcarCelda(int cx, int cz);

    bool estaExplorada(int cx, int cz) const;
    float porcentajeExplorado() const;   // 0.0 .. 1.0

    // Conversion mundo -> celda (sin recortar: puede devolver fuera de rango).
    void mundoACelda(float x, float z, int& cx, int& cz) const;

    // ---- Serializacion RLE (Run-Length Encoding) ----
    // La mascara es un campo enorme (65.536 bytes) pero MUY repetitivo: las
    // celdas exploradas forman manchas continuas, no ruido. Guardar longitudes
    // de tirada alternando "sin explorar / explorado" reduce el archivo a unos
    // pocos miles de numeros sin perder un solo bit de informacion.
    // Por convenio la PRIMERA tirada es de celdas sin explorar (puede valer 0).
    std::vector<uint32_t> comprimirRLE() const;
    bool descomprimirRLE(const std::vector<uint32_t>& tiradas, int ancho, int alto);

    std::size_t celdasExploradas() const { return celdasMarcadasValidas; }
    std::size_t celdasValidas()    const { return totalValidas; }
    int obtenerAncho() const { return ancho; }
    int obtenerAlto()  const { return alto; }
    const std::vector<uint8_t>& obtenerMascara() const { return mascara; }
    const LimitesMundo& obtenerLimites() const { return limites; }
    std::uint64_t obtenerRevision() const { return revision; }

private:
    std::vector<uint8_t> mascara;
    // Cobertura del terreno: denominador honesto del porcentaje. Se copia del
    // Terreno al reiniciar y no cambia mientras dure el mapa.
    std::vector<uint8_t> validas;
    int ancho = 0, alto = 0;
    std::size_t celdasMarcadas = 0;        // todas, incluidas las de relleno
    std::size_t celdasMarcadasValidas = 0; // solo las que pertenecen al terreno
    std::size_t totalValidas = 0;
    LimitesMundo limites;
    std::uint64_t revision = 0;
};
