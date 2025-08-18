'use strict';

import OfflineAudioContext from '../../context/OfflineAudioContext';
import AudioBuffer from '../../api/AudioBuffer';
import AudioWorkletNode from '../../impl/AudioWorkletNode';
import { AudioWorkletProcessor } from '../../api';

export default class TestWorkletProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super(options);
    console.log('constructing TestWorkletProcessor');
  }

  process(...args) {
    console.log('processing audio in TestWorkletProcessor', args);
  }

  static get parameterDescriptors() {
    return [
      {
        name: 'test-param',
        defaultValue: 0.5,
        minValue: 0,
        maxValue: 1,
        automationRate: 'k-rate',
      },
    ];
  }
}

describe('OfflineAudioContext', () => {
  it('should return an OfflineAudioContext instance', () => {
    const context = new OfflineAudioContext(2, 128, 44100);

    expect(context instanceof OfflineAudioContext).toBeTruthy();
    expect(context.length).toBe(128);
    expect(context.sampleRate).toBe(44100);
  });

  it("should emit a 'complete' event with AudioBuffer after rendering", (done) => {
    const context = new OfflineAudioContext(2, 1000, 44100);
    const oncomplete = (e) => {
      const audioBuffer = e.renderedBuffer;

      expect(audioBuffer instanceof AudioBuffer).toBeTruthy();
      expect(audioBuffer.numberOfChannels).toBe(2);
      expect(audioBuffer.length).toBe(1000);
      expect(audioBuffer.sampleRate).toBe(44100);
      done();
    };

    context.oncomplete = oncomplete;

    expect(context.oncomplete).toBe(oncomplete);

    context.startRendering();
  });

  it('should return Promise<AudioBuffer>', () => {
    const context = new OfflineAudioContext(2, 1000, 44100);

    return context.startRendering().then((audioBuffer) => {
      expect(audioBuffer instanceof AudioBuffer).toBeTruthy();
      expect(audioBuffer.numberOfChannels).toBe(2);
      expect(audioBuffer.length).toBe(1000);
      expect(audioBuffer.sampleRate).toBe(44100);
    });
  });

  it("should change context's state in each rendering phase", (done) => {
    const context = new OfflineAudioContext(2, 1000, 44100);

    expect(context.state).toBe('suspended');

    context.startRendering();

    expect(context.state).toBe('running');

    context.oncomplete = () => {
      expect(context.state).toBe('closed');
      done();
    };
  });

  it('should suspend rendering', (done) => {
    const context = new OfflineAudioContext(2, 44100, 44100);

    context.suspend(300 / 44100).then(() => {
      expect(context.state).toBe('suspended');
      context.oncomplete = () => {
        done();
      };
      context.resume();
    });
    context.startRendering();

    context.oncomplete = () => {
      throw new TypeError('SHOULD NOT REACHED');
    };
  });

  it('should register audio worklet', () => {
    const context = new OfflineAudioContext(2, 1000, 44100);
    expect(() =>
      context.audioWorklet.addModule('invalid-path.js', {
        name: 'test-worklet-processor',
        processorCtor: TestWorkletProcessor,
      }),
    ).not.toThrowError();
  });

  it('should create audio worklet node from name/constructor', () => {
    const context = new OfflineAudioContext(2, 1000, 44100);
    context.audioWorklet.addModule('invalid-path.js', {
      name: 'test-worklet-processor',
      processorCtor: TestWorkletProcessor,
    });
    expect(
      () => new AudioWorkletNode(context, 'test-worklet-processor'),
    ).not.toThrowError();
  });

  it('should fail to create audio worklet node with unregistered name', () => {
    const context = new OfflineAudioContext(2, 1000, 44100);
    context.audioWorklet.addModule('invalid-path.js', {
      name: 'test-worklet-processor',
      processorCtor: TestWorkletProcessor,
    });
    expect(() => new AudioWorkletNode(context, 'invalid-name')).toThrowError();
  });

  it('should fail to create audio worklet node with name registered in different audio context', () => {
    const context = new OfflineAudioContext(2, 1000, 44100);
    const context2 = new OfflineAudioContext(2, 1000, 44100);
    context.audioWorklet.addModule('invalid-path.js', {
      name: 'test-worklet-processor',
      processorCtor: TestWorkletProcessor,
    });
    expect(
      () => new AudioWorkletNode(context2, 'test-worklet-processor'),
    ).toThrowError();
  });
});
