import { jest, afterAll } from '@jest/globals';
import { config } from '../config';
import { app, redisClient, serverInstance } from '../index';
import * as supertest from 'supertest';
import type { Redis } from 'ioredis';
import type { Logger } from 'winston';

type DetectLanguageResult = 'jpn' | 'kor' | 'cmn' | 'und' | 'eng';
type DetectLanguageFunction = (text: string) => DetectLanguageResult;

// Set environment variables
process.env.NODE_ENV = 'test';
process.env.PORT = '3001';
process.env.HOST = 'localhost';
process.env.REDIS_HOST = 'localhost';
process.env.REDIS_PORT = '6379';

// Create Redis mock base functions
const redisFunctions = {
  get: jest.fn().mockImplementation(() => Promise.resolve(null)),
  set: jest.fn().mockImplementation(() => Promise.resolve('OK')),
  quit: jest.fn().mockImplementation(() => Promise.resolve()),
  disconnect: jest.fn(),
  on: jest.fn()
};

// Mock Redis implementation
const mockRedis = {
  ...redisFunctions,
  status: 'ready' as const
};

// Mock Redis class
const RedisMock = jest.fn().mockImplementation(() => mockRedis);

// Mock Redis module
jest.mock('ioredis', () => {
  const Redis = jest.fn().mockImplementation(() => {
    return {
      status: 'ready',
      get: jest.fn(),
      set: jest.fn(),
      quit: jest.fn(),
      on: jest.fn(),
      // Add any other Redis methods you need
    };
  });
  
  return Redis;
});

// Define detect function with proper type
const detectLanguage: DetectLanguageFunction = (text) => {
  if (text.includes('日本語')) return 'jpn';
  if (text.includes('한국어')) return 'kor';
  if (text.includes('中文')) return 'cmn';
  if (!text || text.length < 3) return 'und';
  return 'eng';
};

// Mock franc module
jest.mock('franc', () => ({
  __esModule: true,
  detect: jest.fn(detectLanguage)
}));

// Mock Winston logger
const mockLoggerFunctions = {
  info: jest.fn(),
  error: jest.fn(),
  warn: jest.fn(),
  debug: jest.fn(),
  child: jest.fn().mockReturnThis()
};

const mockLogger = mockLoggerFunctions as unknown as Logger;

// Mock Winston module
jest.mock('winston', () => ({
  format: {
    combine: jest.fn(),
    timestamp: jest.fn(),
    json: jest.fn(),
    colorize: jest.fn(),
    simple: jest.fn()
  },
  createLogger: jest.fn().mockReturnValue(mockLogger),
  transports: {
    Console: jest.fn(),
    File: jest.fn()
  }
}));

// Silence console output
const mockConsole = {
  log: jest.fn(),
  error: jest.fn(),
  warn: jest.fn(),
  info: jest.fn(),
  debug: jest.fn()
};

Object.assign(console, mockConsole);

// Make mocks available globally
declare global {
  var __mocks__: {
    redis: typeof mockRedis;
    detectLanguage: DetectLanguageFunction;
  };
}

global.__mocks__ = {
  redis: mockRedis,
  detectLanguage // Export for direct testing if needed
};

// Cleanup function
async function cleanup(): Promise<void> {
  if (serverInstance?.listening) {
    await new Promise<void>((resolve) => {
      serverInstance.close(() => resolve());
    });
  }

  // Clean up Redis connection
  if (redisClient) {
    try {
      await redisClient.quit();
    } catch {
      // Ignore cleanup errors
    }
  }

  // Wait for any pending operations
  await new Promise(resolve => setTimeout(resolve, 100));

  jest.clearAllMocks();
  jest.resetAllMocks();
  jest.restoreAllMocks();
}

// Register cleanup
afterAll(cleanup);

// Export test utilities
export {
  app,
  supertest,
  config,
  mockRedis,
  mockLogger,
  mockConsole,
  cleanup,
  detectLanguage
};
