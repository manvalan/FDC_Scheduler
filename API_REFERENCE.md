# FDC_Scheduler API Reference

Libreria C/C++ per la gestione di reti ferroviarie e orari treni basata su libfdc_core.

## Architettura

La libreria fornisce:
- **C++ API**: Classe `JsonApiWrapper` con metodi che restituiscono JSON
- **C API**: Funzioni `extern "C"` per binding con altri linguaggi
- **JSON-First**: Tutte le operazioni usano JSON per input/output

## Installazione

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

## Linking

```cmake
find_library(FDC_SCHEDULER fdc_scheduler REQUIRED)
target_link_libraries(your_app ${FDC_SCHEDULER})
```

## C API Completa

### 1. Gestione Istanza

#### `fdc_scheduler_create()`
Crea una nuova istanza della libreria.

```c
void* api = fdc_scheduler_create();
```

**Ritorna**: Puntatore opaco all'istanza

---

#### `fdc_scheduler_destroy(void* api)`
Distrugge un'istanza e libera le risorse.

```c
fdc_scheduler_destroy(api);
```

---

### 2. Operazioni su Rete

#### `fdc_scheduler_load_network(void* api, const char* path)`
Carica una rete ferroviaria da file JSON.

**Formato JSON**:
```json
{
  "network": {
    "nodes": [
      {
        "id": "STA",
        "name": "Stazione A",
        "type": "station",
        "latitude": 45.4642,
        "longitude": 9.1900,
        "platforms": 4,
        "capacity": 10
      }
    ],
    "edges": [
      {
        "from": "STA",
        "to": "STB",
        "distance": 126.5,
        "max_speed": 200.0,
        "track_type": "double",
        "bidirectional": true
      }
    ]
  },
  "schedules": [
    {
      "train_id": "ICN500",
      "schedule_id": "ICN500_20240115",
      "stops": [
        {
          "node_id": "STA",
          "arrival": "2024-01-15T08:00:00",
          "departure": "2024-01-15T08:05:00",
          "platform": 1
        }
      ]
    }
  ]
}
```

**Risposta**:
```json
{
  "status": "success",
  "message": "Network loaded successfully",
  "nodes": 3,
  "edges": 6,
  "trains": 2
}
```

---

#### `fdc_scheduler_save_network(void* api, const char* path)`
Salva la rete corrente su file JSON.

**Risposta**:
```json
{
  "status": "success",
  "message": "Network saved successfully",
  "path": "network.json"
}
```

---

#### `fdc_scheduler_get_network_info(void* api)`
Ottiene statistiche sulla rete.

**Risposta**:
```json
{
  "status": "success",
  "version": "FDC_Scheduler 1.0 (using libfdc_core.a)",
  "num_nodes": 3,
  "num_edges": 6,
  "total_track_length_km": 539.0,
  "average_edge_length_km": 89.83,
  "num_trains": 2,
  "track_types": {
    "single": 4,
    "double": 2,
    "high_speed": 0
  }
}
```

---

#### `fdc_scheduler_add_station(void* api, const char* node_json)`
Aggiunge una stazione alla rete.

**Input JSON**:
```json
{
  "id": "STA",
  "name": "Stazione A",
  "type": "station",
  "latitude": 45.4642,
  "longitude": 9.1900,
  "platforms": 4,
  "capacity": 10
}
```

**Tipi validi**: `"station"`, `"junction"`, `"depot"`, `"yard"`, `"interchange"`

**Risposta**:
```json
{
  "status": "success",
  "message": "Station added",
  "node_id": "STA"
}
```

---

#### `fdc_scheduler_add_track(void* api, const char* edge_json)`
Aggiunge una tratta tra due stazioni.

**Input JSON**:
```json
{
  "from": "STA",
  "to": "STB",
  "distance": 126.5,
  "max_speed": 200.0,
  "track_type": "double",
  "bidirectional": true
}
```

**Tipi binario**: `"single"`, `"double"`, `"high_speed"`, `"freight"`

**Risposta**:
```json
{
  "status": "success",
  "message": "Track section added",
  "from": "STA",
  "to": "STB"
}
```

---

#### `fdc_scheduler_get_all_nodes(void* api)`
Ottiene tutte le stazioni.

**Risposta**:
```json
{
  "status": "success",
  "total": 3,
  "nodes": [
    {
      "id": "STA",
      "name": "Stazione A",
      "type": "station",
      "latitude": 45.4642,
      "longitude": 9.19,
      "platforms": 4,
      "capacity": 10
    }
  ]
}
```

---

#### `fdc_scheduler_get_all_edges(void* api)`
Ottiene tutte le tratte.

**Risposta**:
```json
{
  "status": "success",
  "total": 6,
  "edges": [
    {
      "from": "STA",
      "to": "STB",
      "distance": 126.5,
      "max_speed": 200.0,
      "track_type": "double",
      "bidirectional": true
    }
  ]
}
```

---

### 3. Gestione Treni

#### `fdc_scheduler_add_train(void* api, const char* train_json)`
Aggiunge un treno con il suo orario.

**Input JSON**:
```json
{
  "train_id": "ICN500",
  "schedule_id": "ICN500_20240115",
  "stops": [
    {
      "node_id": "STA",
      "arrival": "2024-01-15T08:00:00",
      "departure": "2024-01-15T08:05:00",
      "platform": 1
    },
    {
      "node_id": "STB",
      "arrival": "2024-01-15T09:00:00",
      "departure": "2024-01-15T09:05:00",
      "platform": 2
    }
  ]
}
```

**Formato tempo**: ISO 8601 (`YYYY-MM-DDTHH:MM:SS`)

**Risposta**:
```json
{
  "status": "success",
  "message": "Train added",
  "train_id": "ICN500",
  "num_stops": 2
}
```

---

#### `fdc_scheduler_get_all_trains(void* api)`
Ottiene tutti i treni.

**Risposta**:
```json
{
  "status": "success",
  "total": 2,
  "trains": [
    {
      "train_id": "ICN500",
      "schedule_id": "ICN500_20240115",
      "stops": [...]
    }
  ]
}
```

---

#### `fdc_scheduler_get_train(void* api, const char* train_id)`
Ottiene dettagli di un treno specifico.

**Risposta**:
```json
{
  "status": "success",
  "train": {
    "train_id": "ICN500",
    "schedule_id": "ICN500_20240115",
    "stops": [
      {
        "node_id": "STA",
        "arrival": "2024-01-15T08:00:00",
        "departure": "2024-01-15T08:05:00",
        "platform": 1
      }
    ]
  }
}
```

---

#### `fdc_scheduler_delete_train(void* api, const char* train_id)`
Elimina un treno.

**Risposta**:
```json
{
  "status": "success",
  "message": "Train deleted",
  "train_id": "ICN500"
}
```

---

### 4. Pathfinding

#### `fdc_scheduler_find_shortest_path(void* api, const char* from, const char* to)`
Calcola il percorso più breve tra due stazioni.

**Algoritmo**: Dijkstra tramite Boost Graph

**Risposta**:
```json
{
  "status": "success",
  "from": "STA",
  "to": "STB",
  "nodes": ["STA", "JNC", "STB"],
  "edges": ["STA-JNC", "JNC-STB"],
  "total_distance_km": 143.0,
  "min_travel_time_hours": 0.955
}
```

---

### 5. Validazione e Conflitti

#### `fdc_scheduler_validate_schedule(void* api, const char* train_id)`
Valida uno o più orari.

**train_id**: ID treno o `NULL` per validare tutti

**Risposta (singolo)**:
```json
{
  "status": "success",
  "train_id": "ICN500",
  "valid": true
}
```

**Risposta (tutti)**:
```json
{
  "status": "success",
  "total_trains": 5,
  "valid_trains": 4,
  "invalid_trains": ["REG202"]
}
```

---

#### `fdc_scheduler_detect_conflicts(void* api)`
Rileva conflitti negli orari.

**Stato**: ⏳ In sviluppo - attualmente restituisce array vuoto

**Risposta**:
```json
{
  "status": "success",
  "total": 0,
  "conflicts": [],
  "message": "Conflict detection integration pending"
}
```

---

### 6. Export RailML

#### `fdc_scheduler_export_railml(void* api, const char* path, const char* version)`
Esporta la rete in formato RailML.

**Stato**: ⏳ Non implementato

**version**: `"3.0"` (default) o `"2.5"`

**Risposta**:
```json
{
  "status": "success",
  "message": "RailML export not yet implemented",
  "version": "3.0",
  "path": "network.railml"
}
```

---

## Esempio Completo

```c
#include <stdio.h>

extern "C" {
    void* fdc_scheduler_create();
    void fdc_scheduler_destroy(void* api);
    const char* fdc_scheduler_add_station(void* api, const char* node_json);
    const char* fdc_scheduler_add_track(void* api, const char* edge_json);
    const char* fdc_scheduler_find_shortest_path(void* api, const char* from, const char* to);
}

int main() {
    // Crea API
    void* api = fdc_scheduler_create();
    
    // Aggiungi stazioni
    const char* sta = "{\"id\":\"STA\",\"name\":\"Stazione A\",\"type\":\"station\"}";
    const char* stb = "{\"id\":\"STB\",\"name\":\"Stazione B\",\"type\":\"station\"}";
    printf("%s\n", fdc_scheduler_add_station(api, sta));
    printf("%s\n", fdc_scheduler_add_station(api, stb));
    
    // Aggiungi tratta
    const char* track = "{\"from\":\"STA\",\"to\":\"STB\",\"distance\":100,\"track_type\":\"double\"}";
    printf("%s\n", fdc_scheduler_add_track(api, track));
    
    // Trova percorso
    printf("%s\n", fdc_scheduler_find_shortest_path(api, "STA", "STB"));
    
    // Cleanup
    fdc_scheduler_destroy(api);
    return 0;
}
```

Compila:
```bash
gcc -o example example.c -lfdc_scheduler -lstdc++
```

---

## Gestione Errori

Tutte le funzioni ritornano JSON con campo `"status"`:

**Successo**:
```json
{
  "status": "success",
  ...
}
```

**Errore**:
```json
{
  "status": "error",
  "message": "Train not found: ICN999"
}
```

**Codici di errore comuni**:
- File non trovato
- JSON malformato
- ID duplicato
- Nodo/tratta inesistente
- Tipo enum non valido

---

## Thread Safety

⚠️ **Non thread-safe**: Usare un'istanza per thread o sincronizzare l'accesso.

---

## Performance

- **Load network**: ~10ms per 100 nodi
- **Pathfinding**: ~1ms (Dijkstra ottimizzato)
- **Add station**: ~0.1ms
- **JSON parsing**: Delegato a nlohmann/json (veloce)

---

## Roadmap

### v1.1 (Q1 2024)
- ✅ API JSON completa
- ✅ Pathfinding
- ✅ Load/Save network
- ⏳ Conflict detection
- ⏳ RailML import/export

### v1.2 (Q2 2024)
- Python bindings
- REST API server
- Unit tests completi

### v2.0 (Q3 2024)
- K-shortest paths
- Real-time updates
- Performance monitoring
- Database backend

---

## Supporto

- **Documentazione**: `ARCHITECTURE.md`
- **Esempi**: Vedi `examples/`
- **Issues**: GitHub repository

---

## Licenza

Coerente con FDC core library.
