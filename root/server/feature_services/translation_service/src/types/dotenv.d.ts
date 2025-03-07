declare module 'dotenv' {
  export interface DotenvParseOutput {
    [key: string]: string;
  }

  export interface DotenvConfigOptions {
    path?: string;
    encoding?: string;
    debug?: boolean;
    override?: boolean;
  }

  export interface DotenvConfigOutput {
    parsed?: DotenvParseOutput;
    error?: Error;
  }

  /**
   * Loads `.env` file contents into process.env.
   */
  export function config(options?: DotenvConfigOptions): DotenvConfigOutput;

  /**
   * Parses a string or buffer in the .env format into an object.
   */
  export function parse<T extends DotenvParseOutput = DotenvParseOutput>(
    src: string | Buffer
  ): T;
}

export = dotenv;
