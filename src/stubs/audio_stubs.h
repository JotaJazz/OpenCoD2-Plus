#ifndef STUBS_AUDIO_H
#define STUBS_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *AudioUnit;
typedef void *AUGraph;
typedef void *AUNode;
typedef unsigned int AudioUnitPropertyID;
typedef unsigned int AudioUnitScope;
typedef unsigned int AudioUnitElement;
typedef int OSStatus;
typedef unsigned int UInt32;
typedef struct {
    double mSampleRate;
    unsigned int mFormatID;
    unsigned int mFormatFlags;
    unsigned int mBytesPerPacket;
    unsigned int mFramesPerPacket;
    unsigned int mBytesPerFrame;
    unsigned int mChannelsPerFrame;
    unsigned int mBitsPerChannel;
    unsigned int mReserved;
} AudioStreamBasicDescription;
typedef struct {
    unsigned int componentType;
    unsigned int componentSubType;
    unsigned int componentManufacturer;
    unsigned int componentFlags;
    unsigned int componentFlagsMask;
} ComponentDescription;
typedef void *AudioDeviceID;

OSStatus NewAUGraph(AUGraph *outGraph);
OSStatus DisposeAUGraph(AUGraph graph);
OSStatus AUGraphInitialize(AUGraph graph);
OSStatus AUGraphStart(AUGraph graph);
OSStatus AUGraphStop(AUGraph graph);
OSStatus AUGraphAddNode(AUGraph graph, const ComponentDescription *desc, AUNode *outNode);
OSStatus AUGraphConnectNodeInput(AUGraph graph, AUNode srcNode, unsigned int srcOutput, AUNode destNode, unsigned int destInput);
OSStatus AUGraphNodeInfo(AUGraph graph, AUNode node, ComponentDescription *outDesc, AudioUnit *outUnit);
OSStatus AUGraphOpen(AUGraph graph);
OSStatus AudioUnitSetProperty(AudioUnit unit, AudioUnitPropertyID id, AudioUnitScope scope, AudioUnitElement elem, const void *data, UInt32 size);
OSStatus AudioUnitGetProperty(AudioUnit unit, AudioUnitPropertyID id, AudioUnitScope scope, AudioUnitElement elem, void *data, UInt32 *size);
OSStatus AudioUnitSetParameter(AudioUnit unit, unsigned int paramID, unsigned int scope, unsigned int elem, float value, unsigned int offset);
OSStatus AudioUnitRender(AudioUnit unit, unsigned int *ioActionFlags, const void *inTimeStamp, unsigned int inOutputBusNumber, unsigned int inNumberFrames, void *ioData);
OSStatus AudioOutputUnitStart(AudioUnit unit);
OSStatus AudioOutputUnitStop(AudioUnit unit);
OSStatus AudioDeviceGetProperty(AudioDeviceID dev, unsigned int channel, int isInput, unsigned int propID, unsigned int *size, void *data);
OSStatus AudioDeviceSetProperty(AudioDeviceID dev, const void *when, unsigned int channel, int isInput, unsigned int propID, unsigned int size, const void *data);
OSStatus AudioHardwareGetProperty(unsigned int propID, unsigned int *size, void *data);

#ifdef __cplusplus
}
#endif
#endif
