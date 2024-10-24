export declare function initAudioOutput(config: AudioOutputConfig, handler: AudioOutputHandler): Promise<InitAudioOutputResult>;
export declare function isPlatformSupported(): boolean;
export interface AudioOutputAddon {
    initAudioOutput(config: AudioOutputConfig, handler: AudioOutputHandler): Promise<NativeInitAudioOutputResult>;
}
export interface NativeInitAudioOutputResult {
    dispose(): void;
}
export interface InitAudioOutputResult {
    dispose(): Promise<void>;
}
export type AudioOutputHandler = (outputBuffer: Int16Array) => void;
export interface AudioOutputConfig {
    sampleRate: number;
    channelCount: number;
    bufferDuration?: number;
}
