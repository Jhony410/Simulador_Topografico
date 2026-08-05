#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// ============================================================================
//  SCENE GRAPH (grafo de escena)
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS
//  ---------------------------------------
//  Es un ARBOL n-ario: cada nodo tiene un padre y N hijos, y guarda su
//  transformacion LOCAL (relativa al padre) mas la transformacion MUNDO
//  (acumulada) que se recalcula en cascada con un recorrido en preorden.
//
//  ¿Por que un arbol y no una lista plana de objetos con matriz absoluta?
//
//  1) El dominio ya es jerarquico: una helice se mueve *con* el dron. Con lista
//     plana habria que reescribir a mano la matriz de las 4 helices y del
//     sensor cada vez que el dron se mueve; con el arbol basta cambiar la
//     matriz local del dron y la cascada propaga el cambio a sus 5 hijos.
//
//  2) El recorrido en preorden garantiza que el padre ya tiene su matriz mundo
//     calculada cuando se visita al hijo: una sola pasada O(n), sin ordenar
//     dependencias ni recalcular nada dos veces.
//
//  3) La propiedad por unique_ptr hace que destruir un nodo destruya su subarbol
//     completo, sin punteros colgantes. El puntero crudo 'padre' es solo un
//     enlace observador hacia arriba, por eso no es unique_ptr (evita el ciclo
//     de propiedad que impediria liberar memoria).
//
//  Jerarquia usada en el simulador:
//     raiz -> { terreno, dron -> { helice1..4, sensor }, marcadores, luces }
// ============================================================================
class NodoEscena {
public:
    explicit NodoEscena(std::string nombre = "");

    // Toma la propiedad del hijo y devuelve un observador para poder moverlo
    // luego sin volver a buscarlo en el arbol.
    NodoEscena* agregarHijo(std::unique_ptr<NodoEscena> hijo);

    // Recorrido en preorden: mundo = matrizPadre * local, y baja a los hijos.
    void actualizarTransformaciones(const glm::mat4& matrizPadre);

    void establecerTransformacionLocal(const glm::mat4& m) { transformacionLocal = m; }
    const glm::mat4& obtenerTransformacionLocal() const { return transformacionLocal; }
    const glm::mat4& obtenerTransformacionMundo() const { return transformacionMundo; }

    // Busqueda por nombre en el subarbol (util para depurar y para la sustentacion).
    NodoEscena* buscar(const std::string& nombre);

    const std::string& obtenerNombre() const { return nombre; }
    NodoEscena* obtenerPadre() const { return padre; }
    const std::vector<std::unique_ptr<NodoEscena>>& obtenerHijos() const { return hijos; }
    std::size_t contarDescendientes() const;

private:
    glm::mat4 transformacionLocal{1.0f};
    glm::mat4 transformacionMundo{1.0f};   // calculada en cascada
    std::vector<std::unique_ptr<NodoEscena>> hijos;
    NodoEscena* padre = nullptr;
    std::string nombre;
};
