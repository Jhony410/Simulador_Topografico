#pragma once
#include <cstdint>
#include <queue>
#include <vector>

class MapaExploracion;

// ============================================================================
//  MODELO: escaneo progresivo del terreno con una COLA (std::queue).
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS
//  ---------------------------------------
//  El sensor del dron DETECTA muchas mas celdas por frame de las que conviene
//  MARCAR: si el jugador vuela rapido puede descubrir varios miles de celdas de
//  golpe, y resolverlas todas en el mismo frame produciria un tiron. La cola
//  desacopla las dos velocidades: se encola todo lo detectado y se desencola un
//  lote fijo por frame, de modo que el coste por frame queda acotado.
//
//  ¿Por que FIFO y no otra estructura?
//   - Una PILA (LIFO) revelaria primero lo ultimo detectado; las celdas que el
//     dron sobrevolo al principio podrian quedarse sin marcar indefinidamente
//     mientras siga apareciendo material nuevo. Con FIFO el frente de
//     exploracion avanza como una onda, en el mismo orden en que se descubrio.
//   - Un std::vector borrando por el frente costaria O(n) por desencolado
//     (desplaza todo el arreglo). std::queue sobre deque da O(1) amortizado en
//     ambos extremos, que es justo lo que se necesita.
//   - Una cola de PRIORIDAD daria O(log n) y aqui no hay ninguna celda mas
//     urgente que otra: el orden de llegada ES el criterio correcto.
//
//  Junto a la cola se lleva un arreglo 'enCola' de pertenencia. Sin el, un dron
//  quieto reencolaria las mismas celdas 60 veces por segundo y la cola creceria
//  sin limite. Consultar pertenencia en O(1) con un arreglo es mas barato que
//  recorrer la cola buscando duplicados.
// ============================================================================
class SistemaEscaneo {
public:
    void reiniciar(int ancho, int alto);

    // Encola las celdas aun no exploradas que caen bajo el sensor del dron.
    void detectar(const MapaExploracion& mapa, float x, float z, float radio);

    // Desencola hasta 'lote' celdas y las marca. Devuelve cuantas marco.
    int procesarLote(MapaExploracion& mapa, int lote);

    std::size_t pendientes() const { return cola.size(); }

private:
    std::queue<uint32_t> cola;      // indices aplanados z * ancho + x
    std::vector<uint8_t> enCola;    // pertenencia O(1): evita duplicados
    int ancho = 0, alto = 0;
};
