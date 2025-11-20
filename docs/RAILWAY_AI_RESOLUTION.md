# RailwayAI Conflict Resolution - Documentazione

## Panoramica

Il modulo **RailwayAI Resolver** implementa la risoluzione automatica dei conflitti ferroviari utilizzando algoritmi di intelligenza artificiale. Gestisce tre tipi principali di conflitti:

1. **Conflitti su binario doppio** - Gestione della separazione tra treni sulla stessa tratta
2. **Conflitti su binario singolo** - Risoluzione di collisioni frontali e pianificazione incroci
3. **Conflitti in stazione** - Ottimizzazione assegnazione piattaforme e gestione sovrapposizioni

## Architettura

### Componenti Principali

```
┌─────────────────────────────────────────────────────────────┐
│                    ConflictDetector                         │
│  - Rileva conflitti tra treni                              │
│  - Classifica per tipo e gravità                           │
└──────────────────────┬──────────────────────────────────────┘
                       │ Detected Conflicts
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                  RailwayAIResolver                          │
│  ┌───────────────────────────────────────────────────┐     │
│  │  resolve_double_track_conflict()                  │     │
│  │  - Calcola separazione minima                     │     │
│  │  - Applica ritardi basati su priorità            │     │
│  │  - Valida risoluzione                             │     │
│  └───────────────────────────────────────────────────┘     │
│  ┌───────────────────────────────────────────────────┐     │
│  │  resolve_single_track_conflict()                  │     │
│  │  - Trova punti di incrocio                       │     │
│  │  - Pianifica meet in stazioni                     │     │
│  │  - Gestisce priorità direzioni                    │     │
│  └───────────────────────────────────────────────────┘     │
│  ┌───────────────────────────────────────────────────┐     │
│  │  resolve_station_conflict()                       │     │
│  │  - Riassegna piattaforme disponibili             │     │
│  │  - Ottimizza utilizzo binari                      │     │
│  │  - Applica buffer temporali                       │     │
│  └───────────────────────────────────────────────────┘     │
└──────────────────────┬──────────────────────────────────────┘
                       │ Resolution Results
                       ▼
┌─────────────────────────────────────────────────────────────┐
│              Modified Train Schedules                       │
│  - Orari aggiornati                                        │
│  - Nuove assegnazioni piattaforme                          │
│  - Ritardi minimizzati                                     │
└─────────────────────────────────────────────────────────────┘
```

## Strategie di Risoluzione

### 1. Binario Doppio (Double Track)

Su binari doppi, i treni possono viaggiare simultaneamente in entrambe le direzioni. La risoluzione si concentra su:

**Strategia**: `ADJUST_SPEED` o `DELAY_TRAIN`

```cpp
// Configurazione
RailwayAIConfig config;
config.min_headway_seconds = 120;  // 2 minuti minimo tra treni

// Risoluzione
// - Calcola headway richiesto
// - Determina priorità (passeggeri > merci)
// - Applica ritardo minimo al treno meno prioritario
// - Valida che il conflitto sia risolto
```

**Esempio:**
```
Prima:
  FR9615: Milano 08:00 → Bologna 08:45
  FR9617: Milano 08:05 → Bologna 08:50  [CONFLITTO: headway < 2min]

Dopo:
  FR9615: Milano 08:00 → Bologna 08:45  [Nessuna modifica]
  FR9617: Milano 08:07 → Bologna 08:52  [+2 min ritardo]
```

### 2. Binario Singolo (Single Track)

Su binari singoli, solo un treno alla volta può occupare la sezione. Rischio di collisione frontale.

**Strategia**: `ADD_OVERTAKING_POINT` o `PRIORITY_BASED`

```cpp
// Configurazione
RailwayAIConfig config;
config.allow_single_track_meets = true;
config.single_track_meet_buffer = 300;  // 5 minuti buffer

// Risoluzione
// 1. Trova stazioni intermedie con capacità di incrocio
// 2. Pianifica meet point
// 3. Il treno meno prioritario attende
// 4. L'altro transita e poi il primo prosegue
```

**Esempio:**
```
Prima:
  REG8024: A 10:00 → B 10:25 → C 10:45  [→→→]
  REG8025: C 10:00 → B 10:20 → A 10:45  [←←←]
  
  Conflitto: collisione frontale tra B e C

Dopo:
  REG8024: A 10:00 → B 10:25 → C 10:45  [Nessuna modifica]
  REG8025: C 10:00 → B 10:20 (ATTESA 5min) → A 10:50
           [Attende a B che REG8024 transiti]
```

**Punti di incrocio:**
- Stazioni con 2+ binari
- Passing loops
- Scali di incrocio

### 3. Stazioni (Station Conflicts)

Conflitti quando due treni tentano di usare la stessa piattaforma contemporaneamente.

**Strategia**: `CHANGE_PLATFORM` (preferito) o `DELAY_TRAIN`

```cpp
// Configurazione
RailwayAIConfig config;
config.allow_platform_reassignment = true;
config.optimize_platform_usage = true;
config.platform_buffer_seconds = 180;  // 3 minuti tra treni

// Risoluzione
// 1. Cerca piattaforme alternative disponibili
// 2. Se trovata: riassegna treno meno prioritario
// 3. Altrimenti: ritarda treno per liberare piattaforma
```

**Esempio:**
```
Prima:
  FA8522: Napoli arr 15:05 dep 15:25 [Platform 5]
  IC608:  Napoli arr 15:10 dep 15:30 [Platform 5]  [CONFLITTO]

Dopo (Strategia A - Platform Change):
  FA8522: Napoli arr 15:05 dep 15:25 [Platform 5]
  IC608:  Napoli arr 15:10 dep 15:30 [Platform 8]  [Cambiato]

Dopo (Strategia B - Delay):
  FA8522: Napoli arr 15:05 dep 15:25 [Platform 5]
  IC608:  Napoli arr 15:28 dep 15:48 [Platform 5]  [+18 min]
```

## Sistema di Priorità

I treni hanno priorità diverse per determinare chi subisce modifiche:

```cpp
enum class TrainPriority {
    HIGH_SPEED_PASSENGER = 100,  // Alta velocità passeggeri
    INTERCITY_PASSENGER  = 80,   // Intercity
    REGIONAL_PASSENGER   = 60,   // Regionali
    FREIGHT_FAST         = 40,   // Merci veloci
    FREIGHT_SLOW         = 20    // Merci lente
};
```

**Regole:**
- Treno con priorità maggiore mantiene orario originale
- Treno con priorità minore subisce modifiche
- A parità: distribuzione equa del ritardo

## Configurazione

```cpp
RailwayAIConfig config;

// Pesi obiettivi
config.delay_weight = 1.0;              // Peso minimizzazione ritardi
config.platform_change_weight = 0.5;    // Peso cambi piattaforma
config.reroute_weight = 0.8;            // Peso modifiche percorso
config.passenger_impact_weight = 1.2;   // Peso impatto passeggeri

// Vincoli
config.max_delay_minutes = 30;          // Ritardo massimo accettabile
config.min_headway_seconds = 120;       // Distanza minima tra treni

// Binario singolo
config.allow_single_track_meets = true; // Abilita incroci
config.single_track_meet_buffer = 300;  // Buffer incrocio (5 min)

// Stazioni
config.allow_platform_reassignment = true;  // Abilita cambio binario
config.optimize_platform_usage = true;      // Ottimizza utilizzo
config.platform_change_cost_seconds = 180;  // Costo cambio (3 min)
```

## Utilizzo

### Esempio Base

```cpp
#include <fdc_scheduler/railway_network.hpp>
#include <fdc_scheduler/conflict_detector.hpp>
#include <fdc_scheduler/railway_ai_resolver.hpp>

using namespace fdc_scheduler;

// 1. Crea rete
RailwayNetwork network;
network.add_station("MILANO", "Milano Centrale", 12);
network.add_station("ROMA", "Roma Termini", 24);
network.add_track_section("MILANO", "ROMA", 480.0, 250.0, 
                          TrackType::DOUBLE);

// 2. Crea schedule
std::vector<std::shared_ptr<TrainSchedule>> schedules;
auto train1 = std::make_shared<TrainSchedule>("FR9615");
// ... aggiungi stops ...
schedules.push_back(train1);

// 3. Rileva conflitti
ConflictDetector detector(network);
auto conflicts = detector.detect_all(schedules);

// 4. Risolvi con RailwayAI
if (!conflicts.empty()) {
    RailwayAIConfig config;
    config.allow_platform_reassignment = true;
    config.allow_single_track_meets = true;
    
    RailwayAIResolver resolver(network, config);
    auto result = resolver.resolve_conflicts(schedules, conflicts);
    
    if (result.success) {
        std::cout << "Risolti " << conflicts.size() << " conflitti\n";
        std::cout << "Ritardo totale: " << result.total_delay.count() 
                  << " secondi\n";
        std::cout << "Quality score: " << result.quality_score << "\n";
    }
}
```

### Scenario Avanzato

```cpp
// Gestione scenario complesso multi-treno
RailwayAIResolver resolver(network);

// Imposta priorità personalizzate
for (auto& schedule : schedules) {
    if (schedule->get_train_id().starts_with("FR")) {
        schedule->set_priority(100);  // Alta velocità
    } else if (schedule->get_train_id().starts_with("IC")) {
        schedule->set_priority(80);   // Intercity
    } else {
        schedule->set_priority(60);   // Regionali
    }
}

// Rileva e risolvi
auto conflicts = detector.detect_all(schedules);
auto result = resolver.resolve_conflicts(schedules, conflicts);

// Analizza risultati
std::cout << "Strategie utilizzate:\n";
for (const auto& conflict : conflicts) {
    auto single_result = resolver.resolve_single_conflict(
        conflict, schedules
    );
    std::cout << "  - " << strategy_to_string(single_result.strategy_used)
              << " per " << conflict.location << "\n";
}

// Statistiche
auto stats = resolver.get_statistics();
std::cout << "\nStatistiche:\n";
std::cout << "  Double track resolutions: " 
          << stats["double_track_resolutions"] << "\n";
std::cout << "  Single track resolutions: " 
          << stats["single_track_resolutions"] << "\n";
std::cout << "  Station resolutions: " 
          << stats["station_resolutions"] << "\n";
std::cout << "  Delays applied: " 
          << stats["delays_applied"] << "\n";
std::cout << "  Platforms changed: " 
          << stats["platforms_changed"] << "\n";
```

## Metriche di Qualità

Il sistema calcola un **quality score** (0.0-1.0) per ogni risoluzione:

```
Quality Score = 1.0 
  - (delay_penalty × delay_weight)
  - (platform_changes × platform_weight)
  - (trains_affected × impact_weight)
```

**Interpretazione:**
- `0.9-1.0`: Risoluzione ottimale (minime modifiche)
- `0.7-0.9`: Buona risoluzione (modifiche accettabili)
- `0.5-0.7`: Risoluzione adeguata (modifiche significative)
- `<0.5`: Risoluzione problematica (richiede revisione)

## Best Practices

### 1. Progettazione Rete

✅ **Consigliato:**
- Binario doppio su linee ad alta densità
- Stazioni con 3+ binari su linee singole (per incroci)
- Piattaforme sufficienti nei nodi principali

❌ **Evitare:**
- Binario singolo senza punti di incrocio
- Stazioni con solo 1 binario su linee trafficate

### 2. Schedulazione

✅ **Consigliato:**
- Headway minimo di 2-3 minuti su binario doppio
- Buffer di 5-10 minuti per incroci su binario singolo
- Tempo di sosta realistico (min 2-3 minuti)

❌ **Evitare:**
- Treni troppo ravvicinati
- Tempi di percorrenza irrealistici
- Sovrapposizioni temporali eccessive

### 3. Configurazione AI

Per **linee ad alta velocità**:
```cpp
config.min_headway_seconds = 180;  // 3 minuti
config.max_delay_minutes = 15;     // Ritardi contenuti
config.passenger_impact_weight = 1.5;  // Alta priorità passeggeri
```

Per **linee regionali**:
```cpp
config.allow_single_track_meets = true;
config.single_track_meet_buffer = 300;  // 5 minuti
config.max_delay_minutes = 30;  // Maggiore tolleranza
```

Per **nodi urbani complessi**:
```cpp
config.allow_platform_reassignment = true;
config.optimize_platform_usage = true;
config.platform_buffer_seconds = 120;  // 2 minuti
```

## Limitazioni e Future Implementazioni

### Limitazioni Attuali

- ⚠️ Rerouting non ancora implementato
- ⚠️ Ottimizzazione speed adjustment limitata
- ⚠️ Considera solo conflitti immediati (non cascate)

### Prossimi Sviluppi

1. **Rerouting intelligente**: percorsi alternativi via grafo
2. **Dynamic speed optimization**: regolazione velocità in tempo reale
3. **Multi-objective optimization**: Pareto frontier per trade-offs
4. **Cascade conflict prevention**: prevenzione conflitti a cascata
5. **Machine learning**: apprendimento da risoluzioni passate

## Riferimenti

- **API Reference**: `include/fdc_scheduler/railway_ai_resolver.hpp`
- **Esempi**: `examples/railway_ai_integration_example.cpp`
- **Test**: `tests/test_railway_ai_resolver.cpp`
- **Architettura**: `ARCHITECTURE.md`

## Supporto

Per domande o problemi:
- Issues: GitHub Issues
- Email: support@fdc-scheduler.org
- Documentazione: https://fdc-scheduler.org/docs
