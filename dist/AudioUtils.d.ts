export declare function getSineWave(frequency: number, sampleCount: number, sampleRate: number): Float32Array;
export declare function interleaveChannels(channels: Float32Array[]): Float32Array;
export declare function float32ToInt16Pcm(input: Float32Array): Int16Array;
export declare function clampFloatSample(floatSample: number): number;
