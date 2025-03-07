```mermaid
flowchart TD
    subgraph "Device Layer - PRIORITY"
        FB[Follower Bot]
        CM[Communication Manager] <--> |WebSocket/MQTT| AG
        CM --> LC[Local Cache]
        LC --> RSM[Response Handler]
    end
    
    subgraph "Cloud Layer - MINIMAL PRIORITY"
        AG[API Gateway]
        TS[Translation Service]
        LD[Language Detection]
        RC[Redis Cache]
        
        AG --> TS
        TS --> LD
        TS --> RC
        RC -- "Cache Hit" --> TS
    end
    
    subgraph "Cloud Layer - FUTURE DEVELOPMENT"
        subgraph "Translation Service - Future"
            LB[Load Balancer] --> AG
            AG --> AUTH[Authentication Service]
            
            TS --> TCL[Translation Cache Layer]
            TCL --> RC
            TCL --> DB[(Persistent Storage)]
            
            TS --> TLE[LLM Translation Engine]
            TS --> NLLB[NLLB Model]
            TS --> OPUS[OPUS-MT Model]
            TS --> GCT[Google Cloud Translation]
            
            RC -- "Cache Miss" --> TLE & NLLB & OPUS & GCT
            TLE & NLLB & OPUS & GCT --> TCL
            TCL --> RC & DB
        end
        
        subgraph "Management - Future"
            MS[Monitoring System] --> TCL
            ML[Model Lifecycle Manager] --> TS
        end
    end
    
    %% Styling
    style FB fill:#f8cea9,stroke:#333,stroke-width:2px
    style "Device Layer - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Cloud Layer - MINIMAL PRIORITY" fill:#e6ffef,stroke:#333,stroke-width:2px
    style "Cloud Layer - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
```
