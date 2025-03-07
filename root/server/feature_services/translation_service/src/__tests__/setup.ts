import { config } from '../config';
import { app, redisClient } from '../index';
import supertest from 'supertest';

// Mock Redis for tests
jest.mock('ioredis', () => {
  const Redis = jest.fn(() => ({
    on: jest.fn(),
    get: jest.fn(),
    set: jest.fn(),
    quit: jest.fn(),
    status: 'ready',
    disconnect: jest.fn()
  }));
  return Redis;
});

// Mock franc module
jest.mock('franc', () => {
  return jest.fn((text: string) => {
    if (text.includes('日本語')) return 'jpn';
    if (text.includes('한국어')) return 'kor';
    if (text.includes('中文')) return 'cmn';
    return 'eng';
  });
});

// Mock Winston logger
jest.mock('winston', () => ({
  format: {
    combine: jest.fn(),
    timestamp: jest.fn(),
    json: jest.fn(),
    simple: jest.fn(),
    colorize: jest.fn(),
    prettyPrint: jest.fn()
  },
  createLogger: jest.fn().mockReturnValue({
    info: jest.fn(),
    error: jest.fn(),
    warn: jest.fn(),
    debug: jest.fn(),
    child: jest.fn().mockReturnThis()
  }),
  transports: {
    Console: jest.fn(),
    File: jest.fn()
  }
}));

// Use a different port for tests
process.env.PORT = '3001';

// Global test setup
beforeAll(async () => {
  // Set test environment variables
  process.env.NODE_ENV = 'test';
  process.env.HOST = 'localhost';
  process.env.REDIS_HOST = 'localhost';
  process.env.REDIS_PORT = '6379';

  // Silence console during tests
  global.console = {
    ...console,
    log: jest.fn(),
    error: jest.fn(),
    warn: jest.fn(),
    info: jest.fn()
  };
});

// Global test teardown
afterAll(async () => {
  // Clean up Redis connection
  await redisClient.quit();
  
  // Wait for all pending operations to complete
  await new Promise((resolve) => setTimeout(resolve, 500));
});

// Reset mocks between tests
afterEach(() => {
  jest.clearAllMocks();
  jest.clearAllTimers();
});

// Export for tests
export { app, supertest, config };
