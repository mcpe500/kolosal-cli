declare module 'uuid' {
  export function v4(): string;
  export const v1: () => string;
  export const v3: (name: any, namespace: any) => string;
  export const v5: (name: any, namespace: any) => string;
  export const NIL: string;
  export const parse: (uuid: string) => Uint8Array;
  export const stringify: (buffer: Uint8Array, offset?: number) => string;
  export const validate: (uuid: string) => boolean;
  export const version: (uuid: string) => number;
}