@echo off
echo Descargando tiny_gltf.h (tinygltf, header-only)...
curl -L -o include\tiny_gltf.h "https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h"
if %errorlevel% neq 0 (
    echo [ERROR] No se pudo descargar. Verifica tu conexion a internet.
    echo Descarga manual: https://github.com/syoyo/tinygltf/blob/release/tiny_gltf.h
) else (
    echo [EXITO] include\tiny_gltf.h listo.
    echo Ahora puedes compilar con compilar.bat
)
pause
