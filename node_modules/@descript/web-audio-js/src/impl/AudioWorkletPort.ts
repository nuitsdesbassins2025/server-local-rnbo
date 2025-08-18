import type { MessagePort } from 'worker_threads';

let MC: (new () => { port1: MessagePort; port2: MessagePort }) | undefined;

export function makeMessageChannel() {
  if (!MC) {
    MC =
      // eslint-disable-next-line @typescript-eslint/ban-ts-ignore
      // @ts-ignore
      typeof MessageChannel === 'function'
        ? // eslint-disable-next-line @typescript-eslint/ban-ts-ignore
          // @ts-ignore
          MessageChannel
        : require('worker_threads')?.MessageChannel;
  }
  if (!MC) {
    throw new Error('cannot create message channel');
  }
  return new MC();
}

let nextPort: MessagePort | undefined;

export function setNextPort(port: MessagePort | undefined): void {
  nextPort = port;
}

export function getNextPort(): MessagePort {
  if (!nextPort) {
    throw new Error('no port available for AudioWorkletProcessor');
  }
  return nextPort;
}
