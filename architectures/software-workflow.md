```mermaid
flowchart TD
    subgraph "Phase 1: Follower Bot Hardware - PRIORITY"
        H1[Setup ESP32 Base Configuration]
        H2[Integrate PAM8302 Audio Amplifier]
        H4[Configure PDM Microphone]
        H5[Power Management Implementation]
        
        H1 --> H2
        H1 --> H4
        H1 --> H5
    end

    subgraph "Phase 2: Follower Bot Firmware - PRIORITY"
        F1[Hardware Abstraction Layer]
        F2[System Manager Implementation]
        F3[Feature Manager Setup]
        F4[Communication Manager]
        
        H1 --> F1
        F1 --> F2
        F2 --> F3
        F2 --> F4
        
        subgraph "Feature Implementation - PRIORITY"
            FE1[Voice Engine Development]
            FE3[Interaction Engine Development]
            
            F3 --> FE1
            F3 --> FE3
        end
        
        subgraph "Communication Stack - PRIORITY"
            C1[WiFi Stack Implementation]
            C2[BLE Stack Implementation]
            C3[Protocol Handler Development]
            
            F4 --> C1
            F4 --> C2
            F4 --> C3
        end
    end

    subgraph "Phase 3: Minimal Server Infrastructure - PRIORITY"
        S1[Setup API Gateway]
        S3[Cache Layer Implementation]
        
        F4 --> S1
        S1 --> S3
        
        subgraph "Core Services - PRIORITY"
            CS2[Device Registry]
            CS3[User Management]
            
            S1 --> CS2
            CS2 --> CS3
        end
        
        subgraph "Feature Services - PRIORITY"
            FS2[Translation Service]
            FS3[TTS Service Implementation]
            
            CS3 --> FS2
            FS2 --> FS3
        end
    end

    subgraph "Phase 4: Basic OTA System - PRIORITY"
        O1[Bootloader Development]
        O2[Update Manager Implementation]
        O3[Firmware Signing System]
        
        FS3 --> O1
        O1 --> O2
        O2 --> O3
    end

    subgraph "Phase 5: Follower Bot Testing - PRIORITY"
        T1[Unit Testing]
        T2[Integration Testing]
        T3[System Testing]
        T6[Production Deployment]
        
        O3 --> T1
        T1 --> T2
        T2 --> T3
        T3 --> T6
    end
    
    subgraph "Future Phase: Hardware Extensions"
        H3[Setup SD Card Interface]
        FE2[Expression Engine Development]
    end

    subgraph "Future Phase: Extended Server Infrastructure"
        S2[Database Cluster Configuration]
        S4[Message Queue Setup]
        
        S1 -.-> S2
        S1 -.-> S4
        
        subgraph "Extended Core Services"
            CS1[Authentication Service]
            CS4[Bot Personality Manager]
            
            S1 -.-> CS1
            CS3 -.-> CS4
        end
        
        subgraph "Extended Feature Services"
            FS1[Chat Service]
            FS4[Voice Processing Service]
            FS5[Expression Service]
            
            CS4 -.-> FS1
            FS3 -.-> FS4
            FS4 -.-> FS5
        end
    end

    subgraph "Future Phase: Advanced OTA"
        O4[Version Control Service]
        O5[Device Management System]
        
        O3 -.-> O4
        O4 -.-> O5
    end

    subgraph "Future Phase: Extended Testing"
        T4[Performance Testing]
        T5[Security Testing]
        
        T3 -.-> T4
        T4 -.-> T5
    end
    
    %% Styling
    style "Phase 1: Follower Bot Hardware - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Phase 2: Follower Bot Firmware - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Phase 3: Minimal Server Infrastructure - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Phase 4: Basic OTA System - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Phase 5: Follower Bot Testing - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Future Phase: Hardware Extensions" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style "Future Phase: Extended Server Infrastructure" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style "Future Phase: Advanced OTA" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style "Future Phase: Extended Testing" fill:#f2f2f2,stroke:#333,stroke-width:1px
```
