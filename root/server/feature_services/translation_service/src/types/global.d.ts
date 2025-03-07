declare global {
  namespace NodeJS {
    interface ProcessEnv {
      NODE_ENV: 'development' | 'production' | 'test';
      PORT: string;
      HOST: string;
      REDIS_HOST: string;
      REDIS_PORT: string;
      REDIS_PASSWORD?: string;
    }
  }

  // Add global test mocks
  var __mocks__: {
    redis: {
      on: jest.Mock;
      get: jest.Mock;
      set: jest.Mock;
      quit: jest.Mock;
      status: string;
      disconnect: jest.Mock;
    };
  };
  
  // Add global test utilities
  interface Console {
    log: jest.Mock;
    error: jest.Mock;
    warn: jest.Mock;
    info: jest.Mock;
    debug: jest.Mock;
  }
}

// Export empty to make it a module
export {};
