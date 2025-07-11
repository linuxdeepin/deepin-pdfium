// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_DIB_H_
#define CORE_FPDFAPI_PAGE_CPDF_DIB_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "core/fxcrt/fx_memory_wrappers.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "base/span.h"

class CPDF_Dictionary;
class CPDF_Document;
class CPDF_Stream;
class CPDF_StreamAcc;

struct DIB_COMP_DATA {
  float m_DecodeMin;
  float m_DecodeStep;
  int m_ColorKeyMin;
  int m_ColorKeyMax;
};

namespace fxcodec {
class Jbig2Context;
class ScanlineDecoder;
}  // namespace fxcodec

constexpr size_t kHugeImageSize = 60000000;

class CPDF_DIB final : public CFX_DIBBase {
 public:
  enum class LoadState : uint8_t { kFail, kSuccess, kContinue };

  CONSTRUCT_VIA_MAKE_RETAIN;

  bool Load(CPDF_Document* pDoc, const CPDF_Stream* pStream);

  // CFX_DIBBase:
  bool SkipToScanline(int line, PauseIndicatorIface* pPause) const override;
  uint8_t* GetBuffer() const override;
  const uint8_t* GetScanline(int line) const override;
  void DownSampleScanline(int line,
                          uint8_t* dest_scan,
                          int dest_bpp,
                          int dest_width,
                          bool bFlipX,
                          int clip_left,
                          int clip_width) const override;

  RetainPtr<CPDF_ColorSpace> GetColorSpace() const { return m_pColorSpace; }
  uint32_t GetMatteColor() const { return m_MatteColor; }

  LoadState StartLoadDIBBase(CPDF_Document* pDoc,
                             const CPDF_Stream* pStream,
                             bool bHasMask,
                             const CPDF_Dictionary* pFormResources,
                             const CPDF_Dictionary* pPageResources,
                             bool bStdCS,
                             uint32_t GroupFamily,
                             bool bLoadMask);
  LoadState ContinueLoadDIBBase(PauseIndicatorIface* pPause);
  RetainPtr<CPDF_DIB> DetachMask();

  bool IsJBigImage() const;

 private:
  CPDF_DIB();
  ~CPDF_DIB() override;

  struct JpxSMaskInlineData {
    JpxSMaskInlineData();
    ~JpxSMaskInlineData();

    int width;
    int height;
    std::vector<uint8_t, FxAllocAllocator<uint8_t>> data;
  };

  LoadState StartLoadMask();
  LoadState StartLoadMaskDIB(RetainPtr<const CPDF_Stream> mask);
  bool ContinueToLoadMask();
  LoadState ContinueLoadMaskDIB(PauseIndicatorIface* pPause);
  bool LoadColorInfo(const CPDF_Dictionary* pFormResources,
                     const CPDF_Dictionary* pPageResources);
  bool GetDecodeAndMaskArray(bool* bDefaultDecode, bool* bColorKey);
  RetainPtr<CFX_DIBitmap> LoadJpxBitmap();
  void LoadPalette();
  LoadState CreateDecoder();
  bool CreateDCTDecoder(pdfium::span<const uint8_t> src_span,
                        const CPDF_Dictionary* pParams);
  void TranslateScanline24bpp(uint8_t* dest_scan,
                              const uint8_t* src_scan) const;
  bool TranslateScanline24bppDefaultDecode(uint8_t* dest_scan,
                                           const uint8_t* src_scan) const;
  void ValidateDictParam(const ByteString& filter);
  void DownSampleScanline1Bit(int orig_Bpp,
                              int dest_Bpp,
                              uint32_t src_width,
                              const uint8_t* pSrcLine,
                              uint8_t* dest_scan,
                              int dest_width,
                              bool bFlipX,
                              int clip_left,
                              int clip_width) const;
  void DownSampleScanline8Bit(int orig_Bpp,
                              int dest_Bpp,
                              uint32_t src_width,
                              const uint8_t* pSrcLine,
                              uint8_t* dest_scan,
                              int dest_width,
                              bool bFlipX,
                              int clip_left,
                              int clip_width) const;
  void DownSampleScanline32Bit(int orig_Bpp,
                               int dest_Bpp,
                               uint32_t src_width,
                               const uint8_t* pSrcLine,
                               uint8_t* dest_scan,
                               int dest_width,
                               bool bFlipX,
                               int clip_left,
                               int clip_width) const;
  bool TransMask() const;
  void SetMaskProperties();

  UnownedPtr<CPDF_Document> m_pDocument;
  RetainPtr<const CPDF_Stream> m_pStream;
  RetainPtr<const CPDF_Dictionary> m_pDict;
  RetainPtr<CPDF_StreamAcc> m_pStreamAcc;
  RetainPtr<CPDF_ColorSpace> m_pColorSpace;
  uint32_t m_Family = 0;
  uint32_t m_bpc = 0;
  uint32_t m_bpc_orig = 0;
  uint32_t m_nComponents = 0;
  uint32_t m_GroupFamily = 0;
  uint32_t m_MatteColor = 0;
  LoadState m_Status = LoadState::kFail;
  bool m_bLoadMask = false;
  bool m_bDefaultDecode = true;
  bool m_bImageMask = false;
  bool m_bDoBpcCheck = true;
  bool m_bColorKey = false;
  bool m_bHasMask = false;
  bool m_bStdCS = false;
  std::vector<DIB_COMP_DATA> m_CompData;
  std::unique_ptr<uint8_t, FxFreeDeleter> m_pLineBuf;
  std::unique_ptr<uint8_t, FxFreeDeleter> m_pMaskedLine;
  RetainPtr<CFX_DIBitmap> m_pCachedBitmap;
  // Note: Must not create a cycle between CPDF_DIB instances.
  RetainPtr<CPDF_DIB> m_pMask;
  RetainPtr<CPDF_StreamAcc> m_pGlobalAcc;
  std::unique_ptr<fxcodec::ScanlineDecoder> m_pDecoder;
  JpxSMaskInlineData m_JpxInlineData;

  // Must come after |m_pCachedBitmap|.
  std::unique_ptr<fxcodec::Jbig2Context> m_pJbig2Context;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_DIB_H_
