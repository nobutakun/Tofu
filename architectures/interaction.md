```mermaid
flowchart TD
    subgraph "Follower Bot - PRIORITY"
        F_UI[User Interface] --> F_VIP[Voice Input Processor] & F_TIP[Text Input Processor]
        F_VIP & F_TIP --> F_LLD[Local Language Detector]
        F_LLD --> F_LC[Local Cache]
        
        F_LC -- "Cache Hit" --> F_RSM[Response Handler]
        F_LC -- "Cache Miss" --> F_CM[Communication Manager]
        
        subgraph "Follower Communication"
            F_BLE[BLE Stack] --> F_CSCF[Cloud Service Client]
        end
        
        F_RSM --> F_TTS[Text-to-Speech] & F_UI
        F_OBRF[Offline Basic Responses] --> F_RSM
    end
    
    subgraph "Mobile App - PRIORITY"
        APP[Mobile/PC App]
    end
    
    APP <--> |BLE/Wi-Fi| F_BLE
    
    subgraph "Cloud Layer - MINIMAL PRIORITY"
        Cloud[Cloud Services]
        Cloud --> |Translation Response| F_CSCF
    end
    
    subgraph "Leader Bot - FUTURE DEVELOPMENT"
        L_UI[User Interface] --> L_VIP[Voice Input Processor] & L_TIP[Text Input Processor]
        L_VIP & L_TIP --> L_LLD[Local Language Detector]
        L_LLD --> L_LC[Local Cache]
        
        L_LC -- "Cache Hit" --> L_RSM[Response Handler]
        L_LC -- "Cache Miss" --> L_CM[Communication Manager]
        
        subgraph "Leader Communication"
            L_BLE[BLE Stack] --> L_CSCL[Cloud Service Client]
            L_CSCL <--> |MQTT/HTTPS| Cloud
            L_CSCL --> L_RQML[Request Queue Manager]
            L_RQML --> L_RSM
        end
        
        L_RSM --> L_TTS[Text-to-Speech] & L_UI
        L_OBRL[Offline Basic Responses] --> L_RSM
    end
    
    %% Leader-Follower Interaction - Future
    L_BLE <-.-> |BLE - Future| F_BLE
    L_CM <-.-> |Delegate Task - Future| F_CM
    F_CM <-.-> |Response - Future| L_CM
    
    %% Styling
    style "Follower Bot - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Mobile App - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Cloud Layer - MINIMAL PRIORITY" fill:#e6ffef,stroke:#333,stroke-width:2px
    style "Leader Bot - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style F_BLE fill:#85d4ec,stroke:#333,stroke-width:2px
    style L_BLE fill:#85d4ec,stroke:#333,stroke-width:1px,stroke-dasharray: 5 5
    style APP fill:#d8adf0,stroke:#333,stroke-width:2px
```
