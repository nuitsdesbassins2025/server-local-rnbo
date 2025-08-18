// adapted from https://github.com/GoogleChromeLabs/audioworklet-polyfill/blob/master/src/index.js

import { BaseAudioContext } from '../api';
import AudioWorkletProcessor from './AudioWorkletProcessor';

export interface AudioWorkletOptions {
  outputChannelCount: number[];
  numberOfInputs: number;
  numberOfOutputs: number;

  // for web-audio-js only, register via a processor constructor rather than a url
  name: string;
  processorCtor: typeof AudioWorkletProcessor;
}

const workletProcessors = new WeakMap<
  BaseAudioContext,
  Map<string, typeof AudioWorkletProcessor>
>();

export function getWorkletProcessor(
  audioContext: BaseAudioContext,
  name: string,
): typeof AudioWorkletProcessor | undefined {
  return workletProcessors.get(audioContext)?.get(name);
}

function registerProcessor(
  audioContext: BaseAudioContext,
  name: string,
  Processor: typeof AudioWorkletProcessor,
): void {
  let contextProcessors = workletProcessors.get(audioContext);
  if (!contextProcessors) {
    contextProcessors = new Map();
    workletProcessors.set(audioContext, contextProcessors);
  }
  contextProcessors.set(name, Processor);
}

export default class AudioWorklet {
  constructor(private readonly audioContext: BaseAudioContext) {
    // NOP
  }

  async addModule(_url: string, options?: AudioWorkletOptions): Promise<null> {
    if (options?.processorCtor && options?.name) {
      registerProcessor(this.audioContext, options.name, options.processorCtor);
      return null;
    }

    throw new Error(
      'Cannot load audio worklet via url in web-audio-js. Pass name and processorCtr in options',
    );
  }
}
