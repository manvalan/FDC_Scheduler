# FDC_Scheduler - Build Status

**Data**: 19 novembre 2025  
**Versione**: 1.0.0 (in sviluppo)  
**Stato**: �� In Progress - Build Errors Rimanenti

## ✅ Completato

### Struttura Repository
- ✅ Struttura completa con CMake moderno
- ✅ README.md (753 righe) con documentazione completa
- ✅ Build script (build.sh) con opzioni multiple
- ✅ Git repository inizializzato (4 commit)
- ✅ .gitignore configurato
- ✅ LICENSE (MIT)

### Codice Implementato
- ✅ `conflict_detector.cpp` (500+ righe) - Algoritmi completi per 4 tipi di conflitti
- ✅ `json_api.cpp` (700+ righe) - API JSON completa
- ✅ `railml_parser.cpp` / `railml_exporter.cpp` - Stub per supporto futuro
- ✅ File core migrati da FDC (node, edge, railway_network, schedule, train)
- ✅ Header mancanti copiati (node_type, track_type, train_type, serialization)

### Fix Applicati
- ✅ Namespace corretti (fdc → fdc_scheduler)
- ✅ Include paths corretti
- ✅ Metodi helper privati aggiunti a ConflictDetector
- ✅ Signature corrette per time_windows_overlap
- ✅ Alias TrainStop = ScheduleStop aggiunto
- ✅ Metodi inline rimossi dai .cpp
- ✅ Boost dependency aggiunta

## ⚠️ Problemi Rimanenti

### Errori di Compilazione (20+ errors)

#### 1. Mismatch API TrackType
```cpp
// json_api.cpp:62
edge->set_track_type(edge_data["track_type"].get<std::string>());
//                   ^
// ERROR: TrackType è enum, non string
```

**Soluzione**: Implementare conversione string → TrackType enum

#### 2. RailwayNetwork API Incompatibile
```cpp
// json_api.cpp:76-78
network_->get_nodes().size()   // ERROR: get_nodes() non esiste
network_->get_edges().size()   // ERROR: get_edges() non esiste
network_->get_total_length()   // ERROR: get_total_length() non esiste
```

**Problema**: RailwayNetwork usa Boost Graph Library con API diversa:
- `num_nodes()` invece di `get_nodes()`
- `num_edges()` invece di `get_edges()`
- Nessun metodo per iterare nodi/archi direttamente

**Soluzioni Possibili**:

1. **Refactoring RailwayNetwork** (consigliato)
   - Aggiungere wrapper methods per accesso a nodi/archi
   - Mantenere Boost Graph internamente
   - Esporre API più user-friendly

2. **Riscrivere JsonApi**
   - Usare API esistenti di RailwayNetwork
   - Adattare alle limitazioni Boost Graph

3. **Usare FDC come Dipendenza**
   - Non migrare, ma linkare contro libfdc
   - Creare solo nuovi componenti (RailML, AI)

#### 3. Node/Edge Construction
```cpp
// json_api.cpp:49-50
auto node = std::make_shared<Node>(...);
network_->add_node(node);
// ERROR: add_node vuole const Node&, non shared_ptr
```

**Soluzione**: Dereferenziare prima di passare (già fatto per Edge, fare per Node)

## 📋 Next Steps

### Priorità Alta
1. [ ] Fix TrackType conversion (string ↔ enum)
2. [ ] Aggiungere wrapper methods a RailwayNetwork:
   ```cpp
   std::vector<std::shared_ptr<Node>> get_nodes() const;
   std::vector<std::shared_ptr<Edge>> get_edges() const;
   double get_total_length() const;
   ```
3. [ ] Fix Node construction in json_api.cpp

### Priorità Media
4. [ ] Implementare RailML parser (richiede libxml2/pugixml)
5. [ ] Implementare RailML exporter
6. [ ] Unit tests

### Priorità Bassa
7. [ ] Esempi aggiuntivi
8. [ ] Documentazione Doxygen
9. [ ] CI/CD pipeline

## 🔧 Come Procedere

### Opzione A: Quick Fix (2-3 ore)
1. Implementare conversioni TrackType
2. Aggiungere wrapper methods a RailwayNetwork
3. Fix chiamate in json_api.cpp
4. Testare build

### Opzione B: Refactoring Completo (1-2 giorni)
1. Riprogettare API di RailwayNetwork
2. Rimuovere dipendenza da Boost Graph (opzionale)
3. Creare interfacce pulite e moderne
4. Full test coverage

### Opzione C: Dipendenza Esterna (immediato)
1. Usare FDC come libreria esterna
2. Linkare contro libfdc.a
3. Creare solo nuovi componenti
4. Evitare duplicazione codice

## 📊 Statistiche

- **Righe di codice**: ~2500
- **File creati**: 30+
- **Commit**: 4
- **Tempo stimato per completamento**: 4-8 ore (opzione A)
- **Dipendenze**: Boost (graph), nlohmann/json, CMake 3.15+

## 🎯 Obiettivo Finale

Libreria standalone C++17 per:
- ✅ Gestione rete ferroviaria
- ✅ Scheduling treni
- ✅ Rilevamento conflitti (4 tipi)
- ✅ JSON API completa
- ⏳ RailML 2.x/3.x import/export
- ✅ Integrazione con AI optimizer esterni

## 📝 Note

La migrazione da FDC è complicata dalle dipendenze Boost Graph e dalle differenze API. 
**Raccomandazione**: Procedere con Opzione A (Quick Fix) per avere libreria funzionante, 
poi iterare con miglioramenti incrementali.
