```mermaid
flowchart TD
    subgraph "Device Layer - PRIORITY"
        UI[User Interface]
        VIP[Voice Input Processor]
        TIP[Text Input Processor]
        LLD[Local Language Detector]
        LC[Local Cache]
        TTS[Text-to-Speech]
        
        subgraph "Device Communication - PRIORITY"
            FB[Follower Bot]
            BLE[BLE Communication Stack]
        end
        
        CM[Communication Manager]
        RQM[Request Queue Manager]
        RSM[Response Handler]
    end
    
    subgraph "Network Layer - MINIMAL PRIORITY"
        WS[WebSocket/MQTT]
        NR[Network Resilience]
    end
    
    subgraph "Cloud Layer - MINIMAL PRIORITY"
        subgraph "API Gateway"
            AG[API Gateway]
        end
        
        subgraph "Core Services"
            LD[Language Detection Service]
        end
        
        subgraph "Translation Engine"
            TS[Translation Service]
            RC[Redis Cache]
        end
    end
    
    %% User flow connections
    UI --> VIP & TIP
    VIP & TIP --> LLD
    LLD --> LC
    LC -- "Cache hit" --> RSM
    LC -- "Cache miss" --> CM
    
    %% Network communication
    CM <--> WS
    WS <--> NR
    NR <--> AG
    
    %% Minimal cloud processing
    AG --> LD
    LD --> TS
    TS --> RC
    RC -- "Cache hit" --> TS
    
    %% Return flow
    AG --> WS
    WS --> CM
    CM --> RQM
    RQM --> RSM
    RSM --> TTS & UI
    
    subgraph "Device Layer - FUTURE DEVELOPMENT"
        FB_FUTURE[Feedback Collector]
        
        subgraph "Device Communication - FUTURE"
            LB[Leader Bot]
            F2[Follower Bot 2]
        end
    end
    
    subgraph "Cloud Layer - FUTURE DEVELOPMENT"
        subgraph "API Gateway - Future"
            LB_API[Load Balancer]
            Auth[Authentication]
        end
        
        subgraph "Core Services - Future"
            NLP[NLP Processor]
            CM_CLOUD[Context Manager]
            SA[Sentiment Analysis]
            DM[Data Metrics]
        end
        
        subgraph "Translation Engine - Future"
            NLLB[NLLB Model]
            OPUS[OPUS-MT Model]
            GCT[Google Cloud Translation]
        end
        
        subgraph "Analytics & Learning - Future"
            FB_SERV[Feedback Service]
            QI[Quality Improvement]
            AA[Analytics Aggregator]
        end
        
        subgraph "Management & Monitoring - Future"
            MS[Monitoring System]
            CD[Configuration Distributor]
            ML[Model Lifecycle Manager]
        end
    end
    
    %% Future connections (dotted lines)
    UI -.-> FB_FUTURE
    LB <-.-> BLE
    BLE <-.-> F2
    TS -- "Cache miss (future)" -.-> NLLB & OPUS & GCT
    
    %% Styling
    style FB fill:#f8cea9,stroke:#333,stroke-width:2px
    style "Device Layer - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Device Communication - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Network Layer - MINIMAL PRIORITY" fill:#e6ffef,stroke:#333,stroke-width:2px
    style "Cloud Layer - MINIMAL PRIORITY" fill:#e6ffef,stroke:#333,stroke-width:2px
    style "Device Layer - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style "Cloud Layer - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
```
