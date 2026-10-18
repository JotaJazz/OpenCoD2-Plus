#ifndef MAC_DIRECTX_9_CCOLORCONVERTER_H_H
#define MAC_DIRECTX_9_CCOLORCONVERTER_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct ATI4CompsConverterABGR;
struct ATI4CompsConverterARGB;
struct CColorConverter;
struct StdConverterABGR;
struct StdConverterARGB;

enum ConverterType {
    ATI_4_COMPS_CONVERTER_ARGB = 0,
    ATI_4_COMPS_CONVERTER_ABGR = 1,
    STD_CONVERTER_ARGB = 2,
    STD_CONVERTER_ABGR = 3,
    UNKNOWN_CONVERTER = 4
};

struct ATI4CompsConverterABGR {
    int _placeholder;
};

struct ATI4CompsConverterARGB {
    int _placeholder;
};

struct CColorConverter {
    int _vptr$CColorConverter;
};

struct StdConverterABGR {
    int _placeholder;
};

struct StdConverterARGB {
    int _placeholder;
};

#ifdef __cplusplus
}
#endif
#endif
