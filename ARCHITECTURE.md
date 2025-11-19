# FDC_Scheduler Architecture

## Overview

FDC_Scheduler è una libreria wrapper che estende FDC (Railway Manager) con API JSON e supporto RailML, mantenendo la separazione tra logica core e GUI.

## Strategia Implementativa

### Riutilizzo FDC Core

Invece di duplicare codice, FDC_Scheduler:
- **Usa** `libfdc_core.a` di FDC come dipendenza
- **Wrappa** le classi esistenti con API moderne
- **Aggiunge** funzionalità specifiche (RailML, JSON REST-like API)

### Vantaggi

✅ **Zero duplicazione** - usa il codice testato di FDC  
✅ **Compilazione veloce** - solo wrapper, no core  
✅ **Manutenzione facile** - aggiornamenti FDC automatici  
✅ **API pulita** - interfaccia JSON + C API  

## Struttura

```
FDC_Scheduler/
├── CMakeLists.txt           # Dipende da libfdc_core.a
├── src/
│   ├── json_api_wrapper.cpp # Wrapper JSON per classi FDC
│   ├── railml_parser.cpp    # Parser RailML (stub)
│   └── railml_exporter.cpp  # Exporter RailML (stub)
├── examples/
│   └── simple_example.cpp   # Esempio uso C API
└── include/fdc_scheduler/   # (opzionale - header pubblici)
```

## Dipendenze

1. **FDC Core** (`libfdc_core.a`)
   - RailwayNetwork, Schedule, Train, Edge, Node
   - Path: `../FDC/cpp/build/lib/libfdc_core.a`

2. **nlohmann/json** (FetchContent)
   - v3.11.3

3. **Boost Graph** (system)
   - Richiesto da FDC core

## API

### C++ API (namespace fdc_scheduler)

```cpp
class JsonApiWrapper {
    std::string load_network(const std::string& path);
    std::string get_network_info();
    std::string detect_conflicts();
    std::string export_railml(const std::string& path, const std::string& version);
};
```

### C API (per binding Python/altri linguaggi)

```c
void* fdc_scheduler_create();
void fdc_scheduler_destroy(void* api);
const char* fdc_scheduler_load_network(void* api, const char* path);
const char* fdc_scheduler_get_network_info(void* api);
const char* fdc_scheduler_detect_conflicts(void* api);
```

## Build

```bash
# 1. Compila FDC core (se non già fatto)
cd ../FDC/cpp && ./build.sh --no-gui --no-examples

# 2. Compila FDC_Scheduler
cd ../../FDC_Scheduler
./build.sh

# 3. Esegui esempio
./build/examples/simple_example
```

## Output Build

```
✅ libfdc_scheduler.a     # Libreria statica wrapper
✅ simple_example          # Esempio funzionante
```

## Sviluppo Futuro

### Fase 1: JSON API Completa ✅ DONE
- [x] Wrapper base
- [x] Network info
- [ ] Load/save network JSON
- [ ] Schedule management
- [ ] Conflict detection integration

### Fase 2: RailML Support
- [ ] XML library integration (pugixml/tinyxml2)
- [ ] RailML 2.x parser
- [ ] RailML 3.x parser
- [ ] RailML export

### Fase 3: REST API Server
- [ ] HTTP server (Crow/Beast)
- [ ] WebSocket support
- [ ] Async operations

## Testing

```bash
# Compila e testa
cd build
ctest

# O manualmente
./examples/simple_example
```

## Status

**Build**: ✅ Compila correttamente  
**Tests**: ✅ Simple example funziona  
**Coverage**: ~20% funzionalità base  

---

*Ultima modifica: 19 Novembre 2025*
