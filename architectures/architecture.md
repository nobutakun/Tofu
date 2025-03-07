```mermaid
flowchart TD
    subgraph "Device Layer - PRIORITY"
        A[Hardware Core] --> |I2S| C[Audio Module]
        
        subgraph "Core Firmware"
            E[System Manager] --> F[Hardware Abstraction Layer]
            E --> G[Feature Manager]
            E --> H[Communication Manager]
            
            G --> G2[Voice Engine]
            G --> G3[Interaction Engine]
            
            H --> H1[WiFi Stack]
            H --> H2[BLE Stack]
            H --> H3[Protocol Handler]
        end
        
        FB[Follower Bot] --> |BLE/Wi-Fi| APP[Mobile App/PC App]
        FB --> |Voice/Text Input| LLD[Local Language Detector]
        LLD --> LC[Local Cache]
        LC --> |Cache Hit| RSM[Response Handler]
        LC --> |Cache Miss| CM[Communication Manager]
        RSM --> TTS[Text-to-Speech]
        OB[Offline Basic Responses] --> RSM
    end

    subgraph "Client Layer - PRIORITY"
        R[Mobile App] --> R1[UI/UX Module]
        R --> R2[Device Control]
    end

    subgraph "Cloud Layer - FUTURE DEVELOPMENT"
        I[API Gateway] --> J[Load Balancer]
        
        J --> |Route| K[Service Mesh]
        
        K --> L[Core Services]
        K --> M[Feature Services]
        
        subgraph "Core Services"
            L --> L1[Authentication]
            L --> L2[Device Registry]
        end
        
        subgraph "Feature Services"
            M --> M1[Translation Service]
            M --> M2[Chat Service]
        end

        P[Cache Layer] --> |Redis| K
        Q[Database Cluster] --> |Store| K
    end

    subgraph "Client Layer - FUTURE DEVELOPMENT"
        R --> R3[User Features]
        R --> R4[Settings/Config]
        
        S[Web Dashboard] --> S1[Admin Panel]
        S --> S2[Analytics View]
        S --> S3[Management Tools]
    end

    A <--> |WebSocket/MQTT| I
    R <--> |REST/GraphQL| I
    
    %% Styling
    style FB fill:#f8cea9,stroke:#333,stroke-width:2px
    style APP fill:#d8adf0,stroke:#333,stroke-width:2px
    style "Device Layer - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Client Layer - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Cloud Layer - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style "Client Layer - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
```
