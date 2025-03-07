import { jest, afterAll } from '@jest/globals';
import { config } from '../config';
import { app, redisClient, serverInstance } from '../index';
import supertest from 'supertest';
import type { Redis, RedisKey, RedisValue, RedisOptions } from 'ioredis';
import type { Logger } from 'winston';

type MockFn = jest.Mock<any>;

interface MockRedisClient {
  on: MockFn;
  get: MockFn;
  set: MockFn;
  quit: MockFn;
  status: 'ready' | 'connecting' | 'closed';
  disconnect: MockFn;
}

// Set environment variables
process.env.NODE_ENV = 'test';
process.env.PORT = '3001';
process.env.HOST = 'localhost';
process.env.REDIS_HOST = 'localhost';
process.env.REDIS_PORT = '6379';

// Create mock implementations
const mockQuit = jest.fn().mockImplementation(() => Promise.resolve());
const mockGet = jest.fn().mockImplementation(() => Promise.resolve(null));
const mockSet = jest.fn().mockImplementation(() => Promise.resolve('OK'));

// Mock Redis
const mockRedis: MockRedisClient = {
  on: jest.fn(),
  get: mockGet,
  set: mockSet,
  quit: mockQuit,
  status: 'ready',
  disconnect: jest.fn()
};

// Mock Redis module
jest.mock('ioredis', () => {
  return jest.fn().mockImplementation(() => mockRedis);
});

// Mock franc module
jest.mock('franc', () => {
  return jest.fn((text: string) => {
    if (text.includes('日本語')) return 'jpn';
    if (text.includes('한국어')) return 'kor';
    if (text.includes('中文')) return 'cmn';
    if (!text || text.length < 3) return 'und';
    return 'eng';
  });
});

// Mock Winston logger
const mockLogger = {
  info: jest.fn(),
  error: jest.fn(),
  warn: jest.fn(),
  debug: jest.fn(),
  child: jest.fn().mockReturnThis()
};

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
global.__mocks__ = {
  redis: mockRedis
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
    await Promise.resolve(redisClient.quit());
  }

  // Wait for any pending operations
  await new Promise(resolve => setTimeout(resolve, 100));

  // Reset all mocks
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
  cleanup
};
