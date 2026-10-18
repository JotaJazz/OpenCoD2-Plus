#ifndef MAC_TOOLS_MACEXCEPTIONS_H_H
#define MAC_TOOLS_MACEXCEPTIONS_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct AudioChannelDescription;
struct AudioChannelLayout;
struct ComponentDescription;
struct ComponentRecord;
struct StThreadLock;

struct AudioChannelDescription {
    AudioChannelLabel mChannelLabel;
    UInt32 mChannelFlags;
    Float32 mCoordinates[3];
};

struct AudioChannelLayout {
    AudioChannelLayoutTag mChannelLayoutTag;
    UInt32 mChannelBitmap;
    UInt32 mNumberChannelDescriptions;
    AudioChannelDescription mChannelDescriptions[1];
};

struct ComponentDescription {
    OSType componentType;
    OSType componentSubType;
    OSType componentManufacturer;
    long unsigned int componentFlags;
    long unsigned int componentFlagsMask;
};

struct ComponentRecord {
    long int data[1];
};

struct StThreadLock {
    int _placeholder;
};

#ifdef __cplusplus
}
#endif
#endif
