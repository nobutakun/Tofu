flowchart TD
    subgraph "Device Layer"
        FB[Follower Bot] --> |BLE/Wi-Fi| APP[Mobile App/PC App]
        FB --> |Voice/Text Input| LLD[Local Language Detector]
        LLD --> LC[Local Cache]
        LC --> |Cache Hit| RSM[Response Handler]
        LC --> |Cache Miss| CM[Communication Manager]
        RSM --> TTS[Text-to-Speech]
        OB[Offline Basic Responses] --> RSM
    end

    subgraph "Client Layer"
        APP --> |MQTT/HTTPS| AG[API Gateway]
        APP --> UI[User Interface]
    end

    subgraph "Cloud Layer"
        AG --> LB_API[Load Balancer]
        LB_API --> FS[Feature Services]
        FS --> TS[Translation Service]
        FS --> CHS[Chat Service]
        FS --> NS[Note Summary Service]
        FS --> DB[(Database Cluster)]
    end

    %% Styling
    style FB fill:#f8cea9,stroke:#333,stroke-width:2px
    style APP fill:#d8adf0,stroke:#333,stroke-width:2px
    style AG fill:#9fd5f4,stroke:#333,stroke-width:2px