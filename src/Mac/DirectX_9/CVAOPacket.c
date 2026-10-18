#include "common_types.h"
#include "imports.h"

typedef struct {
    void **vtable;
    bool mEnabled;
    bool mNeedsValidation;
    unsigned short padding;
    GLint mVSize;
    GLenum mVType;
    GLsizei mStride;
    const void *mpStream;
} CBaseVAImpl;

typedef struct {
    void **vtable;
    GLuint *mpVAOID;
    UINT32 mCode;
    UINT32 mReserved0C;
    CBaseVAImpl mColorArray;
    CBaseVAImpl mSecondaryColorArray;
    CBaseVAImpl mNormalArray;
    CBaseVAImpl mVertexArray;
    CBaseVAImpl mTexCoordArrays[8];
    VertexProgramStreamState mGenericArrays[16];
} CVAOPacketImpl;

typedef struct CVAOPacketRbTreeNodeBase {
    int _M_color;
    struct CVAOPacketRbTreeNodeBase *_M_parent;
    struct CVAOPacketRbTreeNodeBase *_M_left;
    struct CVAOPacketRbTreeNodeBase *_M_right;
} CVAOPacketRbTreeNodeBase;

typedef struct {
    CVAOPacketRbTreeNodeBase _M_base;
    UINT32 first;
    CVAOPacketImpl second;
} CVAOPacketRbTreeNode;

typedef struct {
    UINT32 first;
    CVAOPacketImpl second;
} CVAOPacketKeyValue;

typedef struct {
    int _M_key_compare_padding;
    CVAOPacketRbTreeNodeBase _M_header;
    UINT32 _M_node_count;
} CVAOPacketRbTree;

typedef struct {
    void (*deletingDtor)(void *);
    void (*completeDtor)(void *);
    bool (*isFixedFunction)(const CVAOPacket *);
} CVAOPacketVTable;

extern unsigned char COpenGL_sOpenGLE[];
extern VAOStatus CVAOPacket_sVAOStatus;
extern UINT32 CVAOPacket_sCurrentPacket;
extern unsigned char CVAOPacket_sGenericPacket[];
extern unsigned char CVAOPacket_sAllPackets[];
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CVAOPacket[];

extern void *imp_g_VAOID;

void __ZdlPv(void *ptr);
void CBaseVA_Reset(const CBaseVA *_this);
void COpenGLVAO_COpenGLVAO(const COpenGLVAO *_this);
void COpenGLVAO_CreateNewBinding(const COpenGLVAO *_this);
void ZN10COpenGLVAOD2Ev(const COpenGLVAO *_this);
unsigned int COpenGL_SetVAO(const COpenGL *_this, const COpenGLVAO *VAO, int IsFixedFunction, int ForceValidation);
UINT32 COpenGLVAO_GetCode(const COpenGLVAO *_this);
bool ZNK10COpenGLVAOeqERKS_(const COpenGLVAO *_this, const COpenGLVAO *v);
unsigned int COpenGL_ReleaseVAOBinding(const COpenGL *_this, const GLuint *TexID);

void *__Znwm(size_t size);
void __ZSt29_Rb_tree_insert_and_rebalancebPSt18_Rb_tree_node_baseS0_RS_(int insert_left, void *x, void *p, void *header);
void *__ZSt28_Rb_tree_rebalance_for_erasePSt18_Rb_tree_node_baseRS_(void *z, void *header);
void *__ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base(void *node);

static CVAOPacketImpl *CVAOPacket_GetGenericPacket(UINT32 index)
{
    return (CVAOPacketImpl *)(CVAOPacket_sGenericPacket + index * sizeof(CVAOPacketImpl));
}

static CVAOPacketRbTreeNode *CVAOPacket_GetTreeNode(CVAOPacketRbTreeNodeBase *node)
{
    return (CVAOPacketRbTreeNode *)node;
}

static const CVAOPacketRbTreeNode *CVAOPacket_GetConstTreeNode(const CVAOPacketRbTreeNodeBase *node)
{
    return (const CVAOPacketRbTreeNode *)node;
}

static const CVAOPacketVTable *CVAOPacket_GetVTable(const CVAOPacket *packet)
{
    return *(const CVAOPacketVTable *const *)packet;
}

static void CVAOPacket_AdvanceCurrentPacket(void)
{
    UINT32 nextPacket;

    nextPacket = CVAOPacket_sCurrentPacket + 1;
    CVAOPacket_sCurrentPacket = (nextPacket == 1) ? 0 : nextPacket;
}

void CVAOPacket_CVAOPacket(const CVAOPacket *_this);
void ZN10CVAOPacketD2Ev(const CVAOPacket *_this);
void ZN10CVAOPacketD1Ev(const CVAOPacket *_this);
void ZN10CVAOPacketD0Ev(const CVAOPacket *_this);
void CVAOPacket_SetVAO(const CVAOPacket *_this, int bIsCached);
void CVAOPacket_SetGenericVAO(int IsFixedFunction, int ForceValidation);
void CVAOPacket_InitializeGenericVAO(void);
void CVAOPacket_ReleaseBuffer(const void *p, UINT32 Length);
bool CVAOPacket_IsCached(CVAOPacket *v);
void CVAOPacket_Cache(CVAOPacket *v);
void CVAOPacket_Shutdown(void);
static void __static_initialization_and_destruction_0(int __initialize_p, int __priority);
static void GLOBAL__D__ZN10CVAOPacket10sVAOStatusE(void);
static void GLOBAL__I__ZN10CVAOPacket10sVAOStatusE(void);
bool CVAOPacket_IsFixedFunction(const CVAOPacket *_this);
CVAOPacketRbTreeNodeBase *ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE11lower_boundERS1_(const CVAOPacketRbTree *tree, const UINT32 *key);
CVAOPacketRbTreeNodeBase *ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE11upper_boundERS1_(const CVAOPacketRbTree *tree, const UINT32 *key);
void ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE9_M_insertEPSt18_Rb_tree_node_baseSB_RKS3_(void);
void ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE12insert_equalERKS3_(void);
void ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE8_M_eraseEPSt13_Rb_tree_nodeIS3_E(void);

void CVAOPacket_CVAOPacket(const CVAOPacket *_this)
{
    CVAOPacketImpl *packet;

    packet = (CVAOPacketImpl *)_this;
    COpenGLVAO_COpenGLVAO((const COpenGLVAO *)packet);
    packet->vtable = (void**)vtbl_CVAOPacket;
}

void ZN10CVAOPacketD2Ev(const CVAOPacket *_this)
{
    CVAOPacketImpl *packet;

    packet = (CVAOPacketImpl *)_this;
    packet->vtable = (void**)vtbl_CVAOPacket;
    ZN10COpenGLVAOD2Ev((const COpenGLVAO *)packet);
}

void ZN10CVAOPacketD1Ev(const CVAOPacket *_this)
{
    ZN10CVAOPacketD2Ev(_this);
}

void ZN10CVAOPacketD0Ev(const CVAOPacket *_this)
{
    ZN10CVAOPacketD2Ev(_this);
    __ZdlPv((void *)_this);
}

void CVAOPacket_SetVAO(const CVAOPacket *_this, int bIsCached)
{
    bool isFixedFunction;

    CVAOPacket_sVAOStatus = bIsCached ? USING_CACHED_VAO : USING_VIRGIN_VAO;
    isFixedFunction = CVAOPacket_GetVTable(_this)->isFixedFunction(_this);
    COpenGL_SetVAO((const COpenGL *)COpenGL_sOpenGLE,
                   (const COpenGLVAO *)_this,
                   isFixedFunction,
                   0);
}

void CVAOPacket_SetGenericVAO(int IsFixedFunction, int ForceValidation)
{
    CVAOPacket_sVAOStatus = USING_GENERIC_VAO;
    COpenGL_SetVAO((const COpenGL *)COpenGL_sOpenGLE,
                   (const COpenGLVAO *)CVAOPacket_GetGenericPacket(CVAOPacket_sCurrentPacket),
                   IsFixedFunction,
                   ForceValidation);
    CVAOPacket_AdvanceCurrentPacket();
}

void CVAOPacket_InitializeGenericVAO(void)
{
    CVAOPacketRbTree *allPackets;
    CVAOPacketImpl *genericPacket;
    int i;

    allPackets = (CVAOPacketRbTree *)CVAOPacket_sAllPackets;
    genericPacket = CVAOPacket_GetGenericPacket(0);

    allPackets->_M_node_count = 0;
    allPackets->_M_header._M_color = 0;
    allPackets->_M_header._M_parent = NULL;
    allPackets->_M_header._M_left = &allPackets->_M_header;
    allPackets->_M_header._M_right = &allPackets->_M_header;

    genericPacket->mpVAOID = NULL;
    genericPacket->mCode = 0;
    genericPacket->mReserved0C = 0;

    CBaseVA_Reset((const CBaseVA *)&genericPacket->mColorArray);
    CBaseVA_Reset((const CBaseVA *)&genericPacket->mSecondaryColorArray);
    CBaseVA_Reset((const CBaseVA *)&genericPacket->mNormalArray);
    CBaseVA_Reset((const CBaseVA *)&genericPacket->mVertexArray);
    for (i = 0; i < 8; ++i) {
        CBaseVA_Reset((const CBaseVA *)&genericPacket->mTexCoordArrays[i]);
    }

    for (i = 0; i < 16; ++i) {
        genericPacket->mGenericArrays[i].mNeedsValidation = 1;
        genericPacket->mGenericArrays[i].mEnabled = 0;
        genericPacket->mGenericArrays[i].mVSize = 4;
        genericPacket->mGenericArrays[i].mVType = 0x1406;
        genericPacket->mGenericArrays[i].mNormalized = 0;
        genericPacket->mGenericArrays[i].mStride = 0;
        genericPacket->mGenericArrays[i].mpStream = NULL;
    }

    COpenGLVAO_CreateNewBinding((const COpenGLVAO *)genericPacket);
    CVAOPacket_sVAOStatus = USING_GENERIC_VAO;
    COpenGL_SetVAO((const COpenGL *)COpenGL_sOpenGLE,
                   (const COpenGLVAO *)CVAOPacket_GetGenericPacket(CVAOPacket_sCurrentPacket),
                   1,
                   1);
    CVAOPacket_AdvanceCurrentPacket();
    glVertexArrayParameteriAPPLE(0x851f, 0x85bf);
}

static void CopyBaseVA(CBaseVAImpl *dst, const CBaseVAImpl *src)
{
    dst->mEnabled = src->mEnabled;
    dst->mNeedsValidation = src->mNeedsValidation;
    dst->mVSize = src->mVSize;
    dst->mVType = src->mVType;
    dst->mStride = src->mStride;
    dst->mpStream = src->mpStream;
}

static void CopyGenericArray(VertexProgramStreamState *dst, const VertexProgramStreamState *src)
{
    dst->mNeedsValidation = src->mNeedsValidation;
    dst->mEnabled = src->mEnabled;
    dst->mVSize = src->mVSize;
    dst->mVType = src->mVType;
    dst->mNormalized = src->mNormalized;
    dst->mStride = src->mStride;
}

void CVAOPacket_ReleaseBuffer(const void *p, UINT32 Length)
{
    CVAOPacketRbTree *allPackets;
    CVAOPacketRbTreeNodeBase *header;
    CVAOPacketRbTreeNodeBase *iter;
    const unsigned char *pStart;
    const unsigned char *pEnd;

    pStart = (const unsigned char *)p;
    pEnd = pStart + Length;
    allPackets = (CVAOPacketRbTree *)CVAOPacket_sAllPackets;
    header = &allPackets->_M_header;
    iter = allPackets->_M_header._M_left;

    while (iter != header) {
        CVAOPacketRbTreeNode *treeNode;
        CVAOPacketImpl *packet;
        const unsigned char *packetStart;
        int found;
        int j;

        treeNode = CVAOPacket_GetTreeNode(iter);
        packet = &treeNode->second;
        packetStart = (const unsigned char *)packet->mVertexArray.mpStream;
        found = 0;

        if (packetStart >= pStart && packetStart < pEnd) {
            found = 1;
        }

        if (!found) {
            for (j = 0; j < 16; j++) {
                const unsigned char *streamPtr = (const unsigned char *)packet->mGenericArrays[j].mpStream;
                if (streamPtr >= pStart && streamPtr < pEnd) {
                    found = 1;
                    break;
                }
            }
        }

        if (found) {
            CVAOPacketRbTreeNodeBase *nextIter;
            GLuint *g_VAOID_ptr;
            GLuint *packetVAOID;

            g_VAOID_ptr = *(GLuint **)&imp_g_VAOID;
            packetVAOID = packet->mpVAOID;

            if (g_VAOID_ptr && packetVAOID && *g_VAOID_ptr == *packetVAOID) {
                *g_VAOID_ptr = 0xFFFFFFFF;
            }

            COpenGL_ReleaseVAOBinding(
                (const COpenGL *)COpenGL_sOpenGLE,
                (const GLuint *)packetVAOID);
            packet->mpVAOID = NULL;

            nextIter = (CVAOPacketRbTreeNodeBase *)__ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base(iter);

            {
                CVAOPacketRbTreeNodeBase *erased;
                erased = (CVAOPacketRbTreeNodeBase *)__ZSt28_Rb_tree_rebalance_for_erasePSt18_Rb_tree_node_baseRS_(iter, header);

                ((CVAOPacketImpl *)((unsigned char *)erased + 0x14))->vtable = (void**)vtbl_CVAOPacket;
                ZN10COpenGLVAOD2Ev((const COpenGLVAO *)((unsigned char *)erased + 0x14));

                __ZdlPv(erased);
            }

            allPackets->_M_node_count--;

            iter = nextIter;
        } else {
            iter = (CVAOPacketRbTreeNodeBase *)__ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base(iter);
        }
    }
}

bool CVAOPacket_IsCached(CVAOPacket *v)
{
    UINT32 Code;
    CVAOPacketRbTree *allPackets;
    CVAOPacketRbTreeNodeBase *lower;
    CVAOPacketRbTreeNodeBase *upper;
    CVAOPacketRbTreeNodeBase *iter;
    CVAOPacketRbTreeNodeBase *header;

    allPackets = (CVAOPacketRbTree *)CVAOPacket_sAllPackets;
    header = &allPackets->_M_header;

    Code = COpenGLVAO_GetCode((const COpenGLVAO *)v);

    upper = ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE11upper_boundERS1_(allPackets, &Code);
    lower = ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE11lower_boundERS1_(allPackets, &Code);

    for (iter = lower; iter != upper; iter = (CVAOPacketRbTreeNodeBase *)__ZSt18_Rb_tree_incrementPSt18_Rb_tree_node_base(iter)) {
        CVAOPacketImpl *cachedPacket;

        cachedPacket = (CVAOPacketImpl *)((unsigned char *)iter + 0x14);

        if (ZNK10COpenGLVAOeqERKS_((const COpenGLVAO *)cachedPacket, (const COpenGLVAO *)v)) {
            CVAOPacketImpl *vPacket;
            int j;

            if (cachedPacket->mpVAOID == NULL) {
                return 0;
            }

            vPacket = (CVAOPacketImpl *)v;

            vPacket->mpVAOID = cachedPacket->mpVAOID;

            CopyBaseVA(&vPacket->mColorArray, &cachedPacket->mColorArray);

            CopyBaseVA(&vPacket->mSecondaryColorArray, &cachedPacket->mSecondaryColorArray);

            CopyBaseVA(&vPacket->mNormalArray, &cachedPacket->mNormalArray);

            CopyBaseVA(&vPacket->mVertexArray, &cachedPacket->mVertexArray);

            for (j = 0; j < 8; j++) {
                CopyBaseVA(&vPacket->mTexCoordArrays[j], &cachedPacket->mTexCoordArrays[j]);
            }

            for (j = 0; j < 16; j++) {
                CopyGenericArray(&vPacket->mGenericArrays[j], &cachedPacket->mGenericArrays[j]);
            }

            {
                CVAOPacketRbTreeNodeBase *erased;
                erased = (CVAOPacketRbTreeNodeBase *)__ZSt28_Rb_tree_rebalance_for_erasePSt18_Rb_tree_node_baseRS_(iter, header);

                ((CVAOPacketImpl *)((unsigned char *)erased + 0x14))->vtable = (void**)vtbl_CVAOPacket;
                ZN10COpenGLVAOD2Ev((const COpenGLVAO *)((unsigned char *)erased + 0x14));

                __ZdlPv(erased);
            }

            allPackets->_M_node_count--;

            return 1;
        }
    }

    return 0;
}

void CVAOPacket_Cache(CVAOPacket *v)
{

    UINT32 Code;
    unsigned char NewPacketBuf[sizeof(CVAOPacketImpl) + 4];
    unsigned char InsertBuf[sizeof(CVAOPacketImpl) + 4];
    CVAOPacketImpl *newPacket;
    CVAOPacketImpl *insertPacket;
    UINT32 *codePtr;

    Code = COpenGLVAO_GetCode((const COpenGLVAO *)v);

    newPacket = (CVAOPacketImpl *)(NewPacketBuf + 4);
    CVAOPacket_CVAOPacket((const CVAOPacket *)newPacket);

    codePtr = (UINT32 *)NewPacketBuf;
    *codePtr = Code;

    insertPacket = (CVAOPacketImpl *)(InsertBuf + 4);
    CVAOPacket_CVAOPacket((const CVAOPacket *)insertPacket);

    *(UINT32 *)InsertBuf = Code;

    {

        typedef void (*insert_equal_fn)(void *tree, void *kv);
        insert_equal_fn fn = (insert_equal_fn)ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE12insert_equalERKS3_;
        fn(CVAOPacket_sAllPackets, InsertBuf);
    }

    insertPacket->vtable = (void**)vtbl_CVAOPacket;
    ZN10COpenGLVAOD2Ev((const COpenGLVAO *)insertPacket);

    newPacket->vtable = (void**)vtbl_CVAOPacket;
    ZN10COpenGLVAOD2Ev((const COpenGLVAO *)newPacket);
}

void CVAOPacket_Shutdown(void)
{
    CVAOPacketRbTree *allPackets;
    CVAOPacketRbTreeNodeBase *root;
    CVAOPacketRbTreeNodeBase *header;

    allPackets = (CVAOPacketRbTree *)CVAOPacket_sAllPackets;
    header = &allPackets->_M_header;
    root = allPackets->_M_header._M_parent;

    while (root != NULL) {
        CVAOPacketRbTreeNodeBase *left;

        {
            typedef void (*erase_fn)(void *tree, void *node);
            erase_fn fn = (erase_fn)ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE8_M_eraseEPSt13_Rb_tree_nodeIS3_E;
            fn(CVAOPacket_sAllPackets, root->_M_right);
        }

        left = root->_M_left;

        {
            CVAOPacketImpl *packet = (CVAOPacketImpl *)((unsigned char *)root + 0x14);
            packet->vtable = (void**)vtbl_CVAOPacket;
            ZN10COpenGLVAOD2Ev((const COpenGLVAO *)packet);
        }
        __ZdlPv(root);

        root = left;
    }

    allPackets->_M_header._M_left = header;
    allPackets->_M_header._M_parent = NULL;
    allPackets->_M_header._M_right = header;
    allPackets->_M_node_count = 0;

    CVAOPacket_sVAOStatus = UNINITIALIZED_VAO_STATUS;
}

static void __static_initialization_and_destruction_0(int __initialize_p, int __priority)
{
    CVAOPacketRbTree *allPackets;

    if (__priority != 0xffff) {
        return;
    }

    if (__initialize_p == 1) {
        allPackets = (CVAOPacketRbTree *)CVAOPacket_sAllPackets;
        allPackets->_M_node_count = 0;
        allPackets->_M_header._M_color = 0;
        allPackets->_M_header._M_parent = NULL;
        allPackets->_M_header._M_left = &allPackets->_M_header;
        allPackets->_M_header._M_right = &allPackets->_M_header;
        CVAOPacket_CVAOPacket((const CVAOPacket *)CVAOPacket_sGenericPacket);
        return;
    }

    if (__initialize_p == 0) {
        ZN10CVAOPacketD2Ev((const CVAOPacket *)CVAOPacket_sGenericPacket);
        CVAOPacket_Shutdown();
    }
}

void GLOBAL__D__ZN10CVAOPacket10sVAOStatusE(void)
{
    __static_initialization_and_destruction_0(0, 0xffff);
}

void GLOBAL__I__ZN10CVAOPacket10sVAOStatusE(void)
{
    __static_initialization_and_destruction_0(1, 0xffff);
}

bool CVAOPacket_IsFixedFunction(const CVAOPacket *_this)
{
    (void)_this;
    return 1;
}

bool CVAOPacketFixedFunction_IsFixedFunction(const CVAOPacket *_this)
{
    (void)_this;
    return 1;
}

bool CVAOPacketProgrammable_IsFixedFunction(const CVAOPacket *_this)
{
    (void)_this;
    return 0;
}

void ZN23CVAOPacketFixedFunctionD1Ev(void) {}
void ZN23CVAOPacketFixedFunctionD0Ev(void) {}
void ZN22CVAOPacketProgrammableD1Ev(void) {}
void ZN22CVAOPacketProgrammableD0Ev(void) {}

CVAOPacketRbTreeNodeBase *ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE11lower_boundERS1_(const CVAOPacketRbTree *tree, const UINT32 *key)
{
    CVAOPacketRbTreeNodeBase *x;
    CVAOPacketRbTreeNodeBase *y;

    x = tree->_M_header._M_parent;
    y = (CVAOPacketRbTreeNodeBase *)&tree->_M_header;
    while (x) {
        if (CVAOPacket_GetConstTreeNode(x)->first >= *key) {
            y = x;
            x = x->_M_left;
        } else {
            x = x->_M_right;
        }
    }

    return y;
}

CVAOPacketRbTreeNodeBase *ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE11upper_boundERS1_(const CVAOPacketRbTree *tree, const UINT32 *key)
{
    CVAOPacketRbTreeNodeBase *x;
    CVAOPacketRbTreeNodeBase *y;

    x = tree->_M_header._M_parent;
    y = (CVAOPacketRbTreeNodeBase *)&tree->_M_header;
    while (x) {
        if (*key < CVAOPacket_GetConstTreeNode(x)->first) {
            y = x;
            x = x->_M_left;
        } else {
            x = x->_M_right;
        }
    }

    return y;
}

void ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE9_M_insertEPSt18_Rb_tree_node_baseSB_RKS3_(void)
{

    return;
}

void ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE12insert_equalERKS3_(void)
{

    return;
}

void ZNSt8_Rb_treeIjSt4pairIKj10CVAOPacketESt10_Select1stIS3_ESt4lessIjESaIS3_EE8_M_eraseEPSt13_Rb_tree_nodeIS3_E(void)
{

    return;
}

fnptr_t vtbl_CVAOPacket[] = { (fnptr_t)ZN10CVAOPacketD1Ev, (fnptr_t)ZN10CVAOPacketD0Ev, (fnptr_t)CVAOPacket_IsFixedFunction };
