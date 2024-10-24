export declare function initAudioOutput(config: AudioOutputConfig, handler: AudioOutputHandler): Promise<InitAudioOutputResult>;
export declare function isPlatformSupported(): boolean;
export interface InitAudioOutputResult {
    dispose(): Promise<void>;
}
export type AudioOutputHandler = (outputBuffer: Int16Array) => void;
export interface AudioOutputConfig {
    sampleRate: number;
    channelCount: number;
    bufferDuration?: number;
}
