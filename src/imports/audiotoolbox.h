#ifndef IMPORTS_AUDIOTOOLBOX_H
#define IMPORTS_AUDIOTOOLBOX_H

#ifdef __cplusplus
extern "C" {
#endif


int AudioConverterDispose();
int AudioConverterFillBuffer();
int AudioConverterFillComplexBuffer();
int AudioConverterNew();
int AudioConverterReset();
int AudioDeviceGetProperty();
int AudioDeviceGetPropertyInfo();
int AudioDeviceSetProperty();
int AudioHardwareGetProperty();
int AudioOutputUnitStart(COD2_KR);
int AudioOutputUnitStop(COD2_KR);
int AudioUnitAddRenderNotify();
int AudioUnitGetProperty();
int AudioUnitInitialize();
int AudioUnitRemoveRenderNotify();
int AudioUnitRender();
int AudioUnitSetParameter(AudioUnit unit, unsigned int paramID, unsigned int scope, unsigned int elem, float value, unsigned int offset);
int AudioUnitSetProperty();
int AudioUnitUninitialize();


#ifdef __cplusplus
}
#endif
#endif
