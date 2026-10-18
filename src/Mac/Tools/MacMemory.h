#ifndef MAC_TOOLS_MACMEMORY_H_H
#define MAC_TOOLS_MACMEMORY_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct AbsoluteTime;
struct FSCatalogInfo;
struct FSRef;
struct HFSUniStr255;
struct LSItemInfoRecord;
struct UnsignedWide;
struct _D3DGAMMARAMP;

struct AbsoluteTime {
    UInt32 lo;
    UInt32 hi;
};

struct FSCatalogInfo {
    UInt16 nodeFlags;
    FSVolumeRefNum volume;
    UInt32 parentDirID;
    UInt32 nodeID;
    UInt8 sharingFlags;
    UInt8 userPrivileges;
    UInt8 reserved1;
    UInt8 reserved2;
    struct UTCDateTime createDate;
    UTCDateTime contentModDate;
    UTCDateTime attributeModDate;
    UTCDateTime accessDate;
    UTCDateTime backupDate;
    UInt32 permissions[4];
    UInt8 finderInfo[16];
    UInt8 extFinderInfo[16];
    UInt64 dataLogicalSize;
    UInt64 dataPhysicalSize;
    UInt64 rsrcLogicalSize;
    UInt64 rsrcPhysicalSize;
    UInt32 valence;
    TextEncoding textEncodingHint;
};

struct FSRef {
    UInt8 hidden[80];
};

struct HFSUniStr255 {
    FSRef component_id;
    FSRef component_index;
    FSRef h_samp_factor;
    FSRef v_samp_factor;
    FSRef quant_tbl_no;
    FSRef dc_tbl_no;
    FSRef ac_tbl_no;
    const __CFString *width_in_blocks;
    const __CFString *height_in_blocks;
    FSRef DCT_scaled_size;
    const __CFString *downsampled_width;
    const __CFString *downsampled_height;
    KeyStruct *component_needed;
    FSRef MCU_width;
    FSRef MCU_height;
    FSRef MCU_blocks;
    FSRef MCU_sample_width;
    FSRef last_col_width;
    FSRef last_row_height;
    OffhandClass *quant_table;
    const FSRef *dct_table;
};

struct LSItemInfoRecord {
    LSItemInfoFlags flags;
    OSType filetype;
    OSType creator;
    CFStringRef extension;
    CFStringRef iconFileName;
    LSKindID kindID;
};

struct UnsignedWide {
    UInt32 lo;
    UInt32 hi;
};

struct _D3DGAMMARAMP {
    WORD red[256];
    WORD green[256];
    WORD blue[256];
};

#ifdef __cplusplus
}
#endif
#endif
