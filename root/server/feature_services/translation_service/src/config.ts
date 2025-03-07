interface ServerConfig {
  host: string;
  port: number;
}

interface RedisConfig {
  host: string;
  port: number;
  password?: string;
}

interface Config {
  server: ServerConfig;
  redis: RedisConfig;
}

const config: Config = {
  server: {
    host: process.env.HOST || '0.0.0.0',
    port: parseInt(process.env.PORT || '3000', 10)
  },
  redis: {
    host: process.env.REDIS_HOST || 'localhost',
    port: parseInt(process.env.REDIS_PORT || '6379', 10),
    password: process.env.REDIS_PASSWORD
  }
};

// Set server timeout
export const SERVER_TIMEOUT = 5000; // 5 seconds

// Export configuration
export { config };
