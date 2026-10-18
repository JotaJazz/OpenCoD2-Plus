#ifndef IMPORTS_CARBON_H
#define IMPORTS_CARBON_H

#ifdef __cplusplus
extern "C" {
#endif


int AECoerceDesc(COD2_KR);
int AECreateAppleEvent(COD2_KR);
int AECreateDesc(COD2_KR);
int AEDisposeDesc(COD2_KR);
int AEGetDescData(COD2_KR);
int AEGetDescDataSize();
int AEGetParamDesc();
int AEGetParamPtr();
int AEInstallEventHandler();
int AEPutParamDesc(COD2_KR);
int AEPutParamPtr();
int AESend(COD2_KR);
int CreateObjSpecifier(COD2_KR);


#ifdef __cplusplus
}
#endif
#endif
