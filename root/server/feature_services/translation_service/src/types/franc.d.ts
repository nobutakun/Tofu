declare module 'franc' {
  export interface FrancOptions {
    minLength?: number;
    only?: string[];
    whitelist?: string[];
    blacklist?: string[];
  }
  
  export function franc(
    input: string,
    options?: FrancOptions
  ): string;
}
