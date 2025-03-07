```mermaid
flowchart TD
    subgraph "Follower Bot - PRIORITY"
        F_UI[User Interface]
        F_VIP[Voice Input Processor]
        F_TIP[Text Input Processor]
        F_LLD[Local Language Detector]
        F_LC[Local Cache]
        
        F_UI --> F_VIP & F_TIP
        F_VIP & F_TIP --> F_LLD
        F_LLD --> F_LC
        
        F_LC -- "Cache Hit" --> F_RSM[Response Handler]
        F_LC -- "Cache Miss" --> F_OBRF[Offline Basic Responses]
        F_OBRF --> F_RSM
        F_RSM --> F_TTS[Text-to-Speech]
        F_TTS --> F_UI
        
        F_CM[Communication Manager]
        F_CM --> F_LC
    end
    
    subgraph "Mobile App - PRIORITY"
        APP[Mobile/PC App]
    end
    
    APP <--> |BLE/Wi-Fi| F_CM
    
    subgraph "Cloud Layer - MINIMAL PRIORITY"
        Cloud[Cloud Services]
        Cloud --> |Direct Sync| F_CM
    end
    
    subgraph "Leader Bot - FUTURE DEVELOPMENT"
        L_UI[User Interface] --> L_VIP[Voice Input Processor] & L_TIP[Text Input Processor]
        L_VIP & L_TIP --> L_LLD[Local Language Detector]
        L_LLD --> L_LC[Local Cache]
        
        L_LC -- "Cache Hit" --> L_RSM[Response Handler]
        L_LC -- "Cache Miss" --> L_OBRL[Offline Basic Responses]
        
        L_OBRL --> L_RSM
        L_LC -- "Queue Untranslated" --> L_RQML[Request Queue Manager]
        
        L_CM[Communication Manager] --> |Offline Detected| L_UI
        L_CM --> |BLE - Future| F_CM
        
        L_RSM --> |Display| L_UI
        L_RSM --> |TTS Task| L_CM
        
        L_CM --> |Reconnect| Cloud
        Cloud --> |Sync| L_RQML
        L_RQML --> |Update Cache| L_LC
        L_LC -.-> |Sync via BLE - Future| F_LC
    end
    
    %% Styling
    style "Follower Bot - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Mobile App - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Cloud Layer - MINIMAL PRIORITY" fill:#e6ffef,stroke:#333,stroke-width:2px
    style "Leader Bot - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style F_UI fill:#f8cea9,stroke:#333,stroke-width:2px
    style F_CM fill:#85d4ec,stroke:#333,stroke-width:2px
    style APP fill:#d8adf0,stroke:#333,stroke-width:2px
```
