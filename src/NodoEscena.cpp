#include "NodoEscena.h"

NodoEscena::NodoEscena(std::string nombre) : nombre(std::move(nombre)) {}

NodoEscena* NodoEscena::agregarHijo(std::unique_ptr<NodoEscena> hijo) {
    hijo->padre = this;
    NodoEscena* observador = hijo.get();
    hijos.push_back(std::move(hijo));
    return observador;
}

void NodoEscena::actualizarTransformaciones(const glm::mat4& matrizPadre) {
    // Preorden: el padre debe estar resuelto antes de bajar, de lo contrario
    // los hijos usarian la matriz del frame anterior.
    transformacionMundo = matrizPadre * transformacionLocal;
    for (auto& h : hijos) h->actualizarTransformaciones(transformacionMundo);
}

NodoEscena* NodoEscena::buscar(const std::string& objetivo) {
    if (nombre == objetivo) return this;
    for (auto& h : hijos) {
        if (NodoEscena* encontrado = h->buscar(objetivo)) return encontrado;
    }
    return nullptr;
}

std::size_t NodoEscena::contarDescendientes() const {
    std::size_t total = hijos.size();
    for (const auto& h : hijos) total += h->contarDescendientes();
    return total;
}
