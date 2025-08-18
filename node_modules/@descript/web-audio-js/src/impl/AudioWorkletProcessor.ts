import type { MessagePort } from 'worker_threads';
import { getNextPort } from './AudioWorkletPort';
import { AudioWorkletOptions } from './AudioWorklet';

export interface AudioParamDescriptor {
  name: string;
  automationRate?: 'a-rate' | 'k-rate';
  minValue?: number;
  maxValue?: number;
  defaultValue?: number;
}

export default class AudioWorkletProcessor {
  port: MessagePort;

  sampleRate = 0;
  currentTime = 0;

  constructor(_options: Partial<AudioWorkletOptions>) {
    this.port = getNextPort();
  }

  process(
    _input: Float32Array[][],
    _output: Float32Array[][],
    _parameters: Record<string, Float32Array>,
  ): void {
    // subclass should implement
  }

  static parameterDescriptors: AudioParamDescriptor[];
}
