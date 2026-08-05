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
    // Prepara la mascara vacia para una grilla y una caja de mundo dadas.
    void reiniciar(int ancho, int alto, const LimitesMundo& limites);

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

    std::size_t celdasExploradas() const { return celdasMarcadas; }
    int obtenerAncho() const { return ancho; }
    int obtenerAlto()  const { return alto; }
    const std::vector<uint8_t>& obtenerMascara() const { return mascara; }
    const LimitesMundo& obtenerLimites() const { return limites; }

private:
    std::vector<uint8_t> mascara;
    int ancho = 0, alto = 0;
    std::size_t celdasMarcadas = 0;
    LimitesMundo limites;
};
