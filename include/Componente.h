#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

// ============================================================================
//  SISTEMA ORIENTADO A COMPONENTES (COP)
//
//  JUSTIFICACION DE LA ESTRUCTURA DE DATOS
//  ---------------------------------------
//  Cada Entidad guarda sus componentes en un
//      std::unordered_map<std::type_index, std::unique_ptr<Componente>>
//
//  ¿Por que un HashMap y no un vector o una jerarquia de herencia?
//
//  1) La consulta natural del motor es "dame EL componente de tipo T de esta
//     entidad". Con un vector habria que recorrerlo y hacer dynamic_cast en
//     cada acceso: O(n) por consulta y con coste de RTTI en cada elemento.
//     El unordered_map resuelve en O(1) promedio con una sola comparacion.
//
//  2) La clave natural es el TIPO, no un numero. std::type_index es una clave
//     hashable derivada de typeid, asi que el tipo se convierte en la llave y
//     el compilador impide equivocarse de indice (algo que un enum entero si
//     permitiria).
//
//  3) El map garantiza unicidad: una entidad no puede tener dos
//     ComponenteMalla, que es exactamente la regla del dominio.
//
//  4) Frente a la herencia clasica (class Dron : public ObjetoRenderizable,
//     public ObjetoAnimado...), la composicion evita la explosion combinatoria
//     de subclases: se agregan capacidades en tiempo de ejecucion sin tocar
//     ninguna jerarquia.
//
//  Coste que aceptamos: los componentes quedan dispersos en el heap (peor
//  localidad de cache que un ECS por arreglos). Con decenas de entidades, como
//  aqui, es irrelevante frente a la claridad que gana el codigo.
// ============================================================================

// Base de todo componente. Solo existe para dar un destructor virtual comun
// que permita guardarlos como unique_ptr<Componente> sin fugas.
class Componente {
public:
    virtual ~Componente() = default;
};

class Entidad {
public:
    explicit Entidad(std::string nombre = "") : nombre_(std::move(nombre)) {}

    // Construye el componente en su sitio y devuelve un puntero observador.
    // Si ya existia uno del mismo tipo, lo reemplaza (unicidad por tipo).
    template <typename T, typename... Args>
    T* agregarComponente(Args&&... args) {
        static_assert(std::is_base_of<Componente, T>::value,
                      "T debe derivar de Componente");
        auto propio = std::make_unique<T>(std::forward<Args>(args)...);
        T* observador = propio.get();
        componentes_[std::type_index(typeid(T))] = std::move(propio);
        return observador;
    }

    // Devuelve nullptr si la entidad no tiene ese componente.
    template <typename T>
    T* obtenerComponente() {
        auto it = componentes_.find(std::type_index(typeid(T)));
        return (it == componentes_.end()) ? nullptr : static_cast<T*>(it->second.get());
    }

    template <typename T>
    const T* obtenerComponente() const {
        auto it = componentes_.find(std::type_index(typeid(T)));
        return (it == componentes_.end()) ? nullptr : static_cast<const T*>(it->second.get());
    }

    template <typename T>
    bool tieneComponente() const {
        return componentes_.count(std::type_index(typeid(T))) > 0;
    }

    template <typename T>
    void quitarComponente() {
        componentes_.erase(std::type_index(typeid(T)));
    }

    const std::string& obtenerNombre() const { return nombre_; }
    std::size_t cantidadComponentes() const { return componentes_.size(); }

private:
    std::string nombre_;
    std::unordered_map<std::type_index, std::unique_ptr<Componente>> componentes_;
};
