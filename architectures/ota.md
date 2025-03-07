```mermaid
flowchart TD
    subgraph "Device Layer - PRIORITY"
        subgraph "Follower Bot"
            BL[Bootloader] --> APP[Application]
            APP --> UM[Update Manager]
            UM --> SEC[Security Check]
            SEC --> STOR[Storage Controller]
            
            subgraph "Failsafe"
                FB[Fallback Image]
                HC[Health Check]
            end
        end
    end

    subgraph "Cloud Infrastructure - MINIMAL PRIORITY"
        FW[Firmware Storage] --> CDN[Content Delivery Network]
        DB[(Version Database)] --> API[API Server]
        API --> CDN
        
        subgraph "Security"
            FS[Firmware Signing]
        end
    end

    subgraph "Production - PRIORITY"
        PF[Factory Firmware] --> IF[Initial Flashing]
    end

    "Follower Bot" <-->|HTTPS| CDN
    "Follower Bot" <-->|MQTT/HTTPS| API
    
    subgraph "Cloud Infrastructure - FUTURE DEVELOPMENT"
        subgraph "Update Service"
            VS[Version Service] --> API
            RC[Release Control] --> VS
            QA[QA Testing] --> RC
        end
        
        subgraph "Device Management"
            DM[Device Manager] --> API
            DG[Device Groups] --> DM
            ST[Statistics & Telemetry] --> DM
        end
        
        subgraph "Security - Future"
            AS[Authentication Service]
            CS[Certificate Service]
        end
    end

    "Follower Bot" <-.->|Future| ST
    CS -.-> IF
    
    %% Styling
    style "Follower Bot" fill:#f8cea9,stroke:#333,stroke-width:2px
    style "Device Layer - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Production - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Cloud Infrastructure - MINIMAL PRIORITY" fill:#e6ffef,stroke:#333,stroke-width:2px
    style "Cloud Infrastructure - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
```
