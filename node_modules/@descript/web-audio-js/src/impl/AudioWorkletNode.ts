import { AudioWorkletOptions, getWorkletProcessor } from './AudioWorklet';
import { AudioBuffer, BaseAudioContext, ScriptProcessorNode } from '../api';
import type { MessagePort } from 'worker_threads';
import { makeMessageChannel, setNextPort } from './AudioWorkletPort';
import AudioWorkletProcessor from './AudioWorkletProcessor';
import AudioParam from '../api/AudioParam';
import AudioParamImpl from './AudioParam';

function audioBufferToArray(audioBuffer: AudioBuffer): Float32Array[] {
  const out = [];
  for (let i = 0; i < audioBuffer.numberOfChannels; i++) {
    out.push(audioBuffer.getChannelData(i));
  }
  return out;
}

interface WorkletScriptProcessorNode extends ScriptProcessorNode {
  parameters: Map<string, AudioParam>;
  port: MessagePort;
  processor: typeof AudioWorkletProcessor;
  instance: AudioWorkletProcessor;
}

export default function AudioWorkletNode(
  context: BaseAudioContext,
  name: string,
  options: Partial<AudioWorkletOptions> = {},
) {
  const Processor = getWorkletProcessor(context, name);
  if (!Processor) {
    throw new Error(`Cannot initialize unregistered worklet: ${name}`);
  }

  // Configuring channels
  // https://webaudio.github.io/web-audio-api/#configuring-channels-with-audioworkletnodeoptions

  const { numberOfOutputs, numberOfInputs, outputChannelCount } = options;
  if (numberOfInputs === 0 && numberOfOutputs === 0) {
    throw new Error('numberOfInputs and numberOfOutputs must not both be 0');
  }

  let outputChannels: number;

  if (outputChannelCount !== undefined) {
    if (outputChannelCount.some((chCount) => chCount === 0)) {
      throw new Error('output channel count must not have 0');
    }
    if (outputChannelCount.length !== numberOfOutputs) {
      throw new Error('output channel count length must be number of outputs');
    }
    if (numberOfInputs === 1 && numberOfOutputs === 1) {
      outputChannels = outputChannelCount[0];
    } else {
      // in theory, we should:
      // > set the channel count of the kth output of the node to the kth element
      // > of outputChannelCount sequence and return.

      // But we are only supporting one output:
      outputChannels = outputChannelCount[0];
    }
  } else {
    if (numberOfInputs === 1 && numberOfOutputs === 1) {
      // in theory, this would be dynamically updated:
      // > NOTE: For this case, the output chanel count will change to
      // > computedNumberOfChannels dynamically based on the input and the
      // > channelCountMode at runtime.
      outputChannels = 1;
    } else {
      outputChannels = 1;
    }
  }

  // FIXME: should this define number of input channels?
  const scriptProcessor = context.createScriptProcessor(
    // to match the current impl of AudioWorkletProcessor in browsers
    // https://webaudio.github.io/web-audio-api/#rendering-loop
    128,
    outputChannels,
    outputChannels,
  ) as WorkletScriptProcessorNode;

  const scriptProcessorParameters = new Map<string, AudioParam>();

  scriptProcessor.parameters = scriptProcessorParameters;
  if (Processor.parameterDescriptors) {
    for (let i = 0; i < Processor.parameterDescriptors.length; i++) {
      const prop = Processor.parameterDescriptors[i];
      const audioParam = new AudioParam(
        context,
        new AudioParamImpl(context, {
          rate: prop.automationRate,
          defaultValue: prop.defaultValue,
        }),
      );
      scriptProcessorParameters.set(prop.name, audioParam);
    }
  }

  const mc = makeMessageChannel();
  setNextPort(mc.port2);
  const inst = new Processor(options);
  setNextPort(undefined);

  let parameters: Record<string, Float32Array> | undefined;

  function getUpdatedParameters(): Record<string, Float32Array> {
    if (!parameters) {
      parameters = {};
      for (const [key, audioParam] of scriptProcessorParameters.entries()) {
        const size =
          audioParam.automationRate === 'k-rate'
            ? 1
            : scriptProcessor.bufferSize;
        parameters[key] = new Float32Array(size);
      }
    }

    for (const [key, audioParam] of scriptProcessorParameters.entries()) {
      const arr = parameters[key];
      arr.fill(audioParam.value);
    }

    return parameters;
  }

  scriptProcessor.port = mc.port1;
  scriptProcessor.processor = Processor;
  scriptProcessor.instance = inst;
  scriptProcessor.onaudioprocess = function onAudioProcess(e: {
    inputBuffer: AudioBuffer;
    outputBuffer: AudioBuffer;
  }) {
    inst.sampleRate = context.sampleRate;
    inst.currentTime = context.currentTime;

    const inputs = audioBufferToArray(e.inputBuffer);
    const outputs = audioBufferToArray(e.outputBuffer);

    inst.process([inputs], [outputs], getUpdatedParameters());
  };
  return scriptProcessor;
}
