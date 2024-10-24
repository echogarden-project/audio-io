export declare function initAudioOutput(config: AudioOutputConfig, handler: AudioOutputHandler): Promise<InitAudioOutputResult>;
export declare function isPlatformSupported(): boolean;
export interface AudioOutputAddon {
    initAudioOutput(config: AudioOutputConfig, handler: AudioOutputHandler): Promise<InitAudioOutputResult>;
}
export interface InitAudioOutputResult {
    dispose(): void;
}
export type AudioOutputHandler = (outputBuffer: Int16Array) => void;
export interface AudioOutputConfig {
    sampleRate: number;
    channelCount: number;
    bufferDuration?: number;
}
