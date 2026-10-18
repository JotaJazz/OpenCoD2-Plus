#ifndef IMPORTS_CARBON_H
#define IMPORTS_CARBON_H

#ifdef __cplusplus
extern "C" {
#endif


int AECoerceDesc();
int AECreateAppleEvent();
int AECreateDesc();
int AEDisposeDesc();
int AEGetDescData();
int AEGetDescDataSize();
int AEGetParamDesc();
int AEGetParamPtr();
int AEInstallEventHandler();
int AEPutParamDesc();
int AEPutParamPtr();
int AESend();
int CreateObjSpecifier();


#ifdef __cplusplus
}
#endif
#endif
