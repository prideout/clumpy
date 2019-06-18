// [Blend2D]
// 2D Vector Graphics Powered by a JIT Compiler.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#ifndef BLEND2D_RASTER_BLRASTERCONTEXT_P_H
#define BLEND2D_RASTER_BLRASTERCONTEXT_P_H

#include "../blapi-internal_p.h"
#include "../blcompop_p.h"
#include "../blcontext_p.h"
#include "../blfont_p.h"
#include "../blgradient_p.h"
#include "../blmatrix_p.h"
#include "../blpath_p.h"
#include "../blpiperuntime_p.h"
#include "../blrgba.h"
#include "../blregion_p.h"
#include "../blsupport_p.h"
#include "../blthreading_p.h"
#include "../blthreadpool_p.h"
#include "../blzoneallocator_p.h"
#include "../raster/blanalyticrasterizer_p.h"
#include "../raster/bledgebuilder_p.h"
#include "../raster/blrasterdefs_p.h"
#include "../raster/blrasterfiller_p.h"
#include "../raster/blrasterworker_p.h"

#if !defined(BL_BUILD_NO_JIT)
  #include "../pipegen/blpipegenruntime_p.h"
#endif

//! \cond INTERNAL
//! \addtogroup blend2d_internal_raster
//! \{

// ============================================================================
// [BLRasterAsyncData]
// ============================================================================

struct BLRasterAsyncData {
  BLThreadPool* threadPool;
  BLThread** threads;
  uint32_t threadCount;

  BL_INLINE BLRasterAsyncData() noexcept
    : threadPool(nullptr),
      threads(nullptr),
      threadCount(0) {}

  BL_INLINE void init(BLThreadPool* threadPool_, BLThread** threads_, uint32_t count_) noexcept {
    this->threadPool = threadPool_;
    this->threads = threads_;
    this->threadCount = count_;
  }

  BL_INLINE void release() noexcept {
    if (!threadPool)
      return;

    if (threadCount)
      threadPool->releaseThreads(threads, threadCount);

    threadPool->release();
    threadPool = nullptr;
    threads = nullptr;
    threadCount = 0;
  }
};

// ============================================================================
// [BLRasterContextImpl]
// ============================================================================

//! Raster rendering context implementation.
struct BLRasterContextImpl : public BLContextImpl {
  //! Zone allocator used to allocate base data structures required by `BLRasterContextImpl`.
  BLZoneAllocator baseZone;
  //! Zone allocator used to allocate commands for deferred and asynchronous rendering.
  BLZoneAllocator cmdZone;
  //! Object pool used to allocate `BLRasterFetchData`.
  BLZonePool<BLRasterFetchData> fetchPool;
  //! Object pool used to allocate `BLRasterContextSavedState`.
  BLZonePool<BLRasterContextSavedState> statePool;

  //! Destination image.
  BLImageCore dstImage;
  //! Destination info.
  BLRasterContextDstInfo dstInfo;

  //! Worker used by synchronous rendering that also holds part of the current state.
  BLRasterWorker worker;
  //! Asynchronous rendering data.
  BLRasterAsyncData asyncData;

  //! Pipeline runtime (either global or isolated, depending on create options).
  BLPipeProvider pipeProvider;
  //! Pipeline lookup cache (always used before attempting to use the `pipeProvider`.
  BLPipeLookupCache pipeLookupCache;

  //! Temporary text buffer used to shape a text to fill.
  BLGlyphBuffer glyphBuffer;

  //! Context origin ID used in `data0` member of `BLContextCookie`.
  uint64_t contextOriginId;
  //! Used to genearate unique IDs of this context.
  uint64_t stateIdCounter;

  //! The current state of the context accessible from outside.
  BLContextState currentState;
  //! Link to the previous saved state that will be restored by `BLContext::restore()`.
  BLRasterContextSavedState* savedState;

  //! Context flags.
  uint32_t contextFlags;

  //! Fixed point shift (able to multiply / divide by fpScale).
  int fpShiftI;
  //! Fixed point scale as int (either 256 or 65536).
  int fpScaleI;
  //! Fixed point mask calculated as `fpScaleI - 1`.
  int fpMaskI;

  //! Fixed point scale as double (either 256.0 or 65536.0).
  double fpScaleD;
  //! Minimum safe coordinate for integral transformation (scaled by 256.0 or 65536.0).
  double fpMinSafeCoordD;
  //! Maximum safe coordinate for integral transformation (scaled by 256.0 or 65536.0).
  double fpMaxSafeCoordD;
  //! Curve flattening tolerance scaled by `fpScaleD`.
  double toleranceFixedD;

  //! Fill and stroke styles.
  BLRasterContextStyleData style[BL_CONTEXT_OP_TYPE_COUNT];

  //! Composition operator simplification that matches the destination format and current `compOp`.
  const BLCompOpSimplifyInfo* compOpSimplifyTable;
  //! Solid format table used to select the best pixel format for solid fills.
  uint8_t solidFormatTable[BL_RASTER_CONTEXT_SOLID_FORMAT_COUNT];

  //! Type of meta matrix.
  uint8_t metaMatrixType;
  //! Type of final matrix.
  uint8_t finalMatrixType;
  //! Type of meta matrix that scales to fixed point.
  uint8_t metaMatrixFixedType;
  //! Type of final matrix that scales to fixed point.
  uint8_t finalMatrixFixedType;
  //! Global alpha as integer (0..256 or 0..65536).
  uint32_t globalAlphaI;

  //! Meta clip-box (int).
  BLBoxI metaClipBoxI;
  //! Final clip box (int).
  BLBoxI finalClipBoxI;
  //! Final clip-box (double).
  BLBox finalClipBoxD;

  //! Result of `(metaMatrix * userMatrix)`.
  BLMatrix2D finalMatrix;
  //! Meta matrix scaled by `fpScale`.
  BLMatrix2D metaMatrixFixed;
  //! Result of `(metaMatrix * userMatrix) * fpScale`.
  BLMatrix2D finalMatrixFixed;

  //! Integral offset to add to input coordinates in case integral transform is ok.
  BLPointI translationI;

  //! Static buffer used by `baseZone` for the first block.
  uint8_t staticBuffer[2048];

  inline BLRasterContextImpl(BLContextVirt* inVirt, uint16_t inMemPoolData) noexcept
    : baseZone(8192 - BLZoneAllocator::kBlockOverhead, 16, staticBuffer, sizeof(staticBuffer)),
      cmdZone(16384 - BLZoneAllocator::kBlockOverhead, 8),
      fetchPool(&baseZone),
      statePool(&baseZone),
      dstImage {},
      dstInfo {},
      worker(this),
      asyncData {},
      pipeProvider(),
      glyphBuffer(),
      contextOriginId(blContextIdGenerator.next()),
      stateIdCounter(0) {

    // Initializes BLContext2DImpl.
    virt = inVirt;
    state = &currentState;
    reservedHeader = nullptr;
    refCount = 1;
    implType = uint8_t(BL_IMPL_TYPE_CONTEXT);
    implTraits = uint8_t(BL_IMPL_TRAIT_MUTABLE | BL_IMPL_TRAIT_VIRT);
    memPoolData = inMemPoolData;
    contextType = BL_CONTEXT_TYPE_RASTER;
    targetSize.reset();
  }

  inline ~BLRasterContextImpl() noexcept {}

  BL_INLINE const BLBox& finalClipBoxFixedD() const noexcept { return worker.edgeBuilder._clipBoxD; }
  BL_INLINE void setFinalClipBoxFixedD(const BLBox& clipBox) { worker.edgeBuilder.setClipBox(clipBox); }
};

// ============================================================================
// [BLRasterContextImpl - API]
// ============================================================================

BL_HIDDEN BLResult blRasterContextImplCreate(BLContextImpl** out, BLImageCore* image, const BLContextCreateInfo* options) noexcept;
BL_HIDDEN void blRasterContextRtInit(BLRuntimeContext* rt) noexcept;

//! \}
//! \endcond

#endif // BLEND2D_RASTER_BLRASTERCONTEXT_P_H
