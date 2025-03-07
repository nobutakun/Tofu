```mermaid
flowchart TD
    subgraph "Follower Bot Support - PRIORITY"
        API[API Gateway]
        API --> |Route| SM[Service Mesh]
        
        subgraph "Minimal Data Storage"
            SM <--> CACHE[Cache Layer]
            CACHE --> C3[Translation Cache]
        end
        
        subgraph "Core Services - PRIORITY"
            SM --> CS1[Device Registry]
            SM --> CS2[User Management]
        end
        
        subgraph "Feature Services - PRIORITY"
            SM --> FS2[Translation Service]
            SM --> FS3[TTS Service]
            
            FS3 --> TTS1[Piper TTS]
        end
        
        subgraph "Management Services - PRIORITY"
            SM --> MS1[OTA Update Service]
            MS1 --> FW[Firmware Repository]
        end
    end
    
    subgraph "Infrastructure Layer - MINIMAL PRIORITY"
        LB[Load Balancer Cluster] --> |Route Traffic| API
        
        subgraph "Additional Data Storage"
            SM <--> DB[(Database Cluster)]
            DB --> DB1[(User Database)]
            DB --> DB2[(Device Database)]
        end
    end
    
    subgraph "Infrastructure Layer - FUTURE DEVELOPMENT"
        API --> |Authenticate| AUTH[Authentication Service]
        
        subgraph "Extended Data Storage"
            SM <--> MQ[Message Queue]
            SM <--> BLOB[Blob Storage]
            
            DB --> DB3[(Conversation History)]
            DB --> DB4[(Bot Profiles)]
            
            CACHE --> C1[Session Cache]
            CACHE --> C2[Response Cache]
        end
        
        subgraph "Core Services - Future"
            SM --> CS3[Bot Personality Manager]
            SM --> CS4[Analytics Engine]
        end
        
        subgraph "Feature Services - Future"
            SM --> FS1[Chat Service]
            SM --> FS4[Voice Processing]
            SM --> FS5[Expression Service]
            
            FS3 --> TTS2[Coqui TTS]
            FS3 --> TTS3[HuggingFace TTS]
        end
        
        subgraph "Management Services - Future"
            SM --> MS2[Configuration Service]
            SM --> MS3[Logging Service]
            SM --> MS4[Health Monitoring]
        end
    end
    
    subgraph "DevOps Infrastructure - FUTURE DEVELOPMENT"
        CI[CI/CD Pipeline]
        MON[Monitoring Stack]
        LOG[Centralized Logging]
        BACKUP[Backup System]
        SEC[Security Monitoring]
    end
    
    subgraph "E-commerce System - FUTURE DEVELOPMENT" 
        ECOM[E-commerce Platform]
        ECOM --> ECART[Shopping Cart]
        ECOM --> EPAY[Payment Processing]
        ECOM --> EINV[Inventory Management]
    end
    
    API <-.-> |Future| ECOM
    
    %% Styling
    style "Follower Bot Support - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Core Services - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Feature Services - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Management Services - PRIORITY" fill:#e6f7ff,stroke:#333,stroke-width:2px
    style "Infrastructure Layer - MINIMAL PRIORITY" fill:#e6ffef,stroke:#333,stroke-width:2px
    style "Infrastructure Layer - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style "DevOps Infrastructure - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
    style "E-commerce System - FUTURE DEVELOPMENT" fill:#f2f2f2,stroke:#333,stroke-width:1px
```
