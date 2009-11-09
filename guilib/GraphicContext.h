/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*!
\file GraphicContext.h
\brief
*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even whith optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif


#include <vector>
#include <stack>
#include <map>
#include "utils/CriticalSection.h"  // base class
#include "TransformMatrix.h"        // for the members m_guiTransform etc.
#include "Geometry.h"               // for XbmcCRect/XbmcCPoint
#include "gui3d.h"
#include "StdString.h"
#include "Resolution.h"

enum VIEW_TYPE { VIEW_TYPE_NONE = 0,
                 VIEW_TYPE_LIST,
                 VIEW_TYPE_ICON,
                 VIEW_TYPE_BIG_LIST,
                 VIEW_TYPE_BIG_ICON,
                 VIEW_TYPE_WIDE,
                 VIEW_TYPE_BIG_WIDE,
                 VIEW_TYPE_WRAP,
                 VIEW_TYPE_BIG_WRAP,
                 VIEW_TYPE_AUTO,
                 VIEW_TYPE_MAX };


class CGraphicContext : public CCriticalSection
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);

  // the following two functions should wrap any
  // GL calls to maintain thread safety
  void BeginPaint(bool lock=true);
  void EndPaint(bool lock=true);

  int GetWidth() const { return m_iScreenWidth; }
  int GetHeight() const { return m_iScreenHeight; }
  float GetFPS() const;
  const CStdString& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const CStdString& strMediaDir);
  bool IsWidescreen() const { return m_bWidescreen; }
  bool SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious = false);
  void RestoreViewPort();
  const XbmcCRect& GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom);
  bool IsFullScreenRoot() const;
  bool ToggleFullScreenRoot();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  bool IsValidResolution(int res);
  void SetVideoResolution(int res, bool forceUpdate = false);
  int GetVideoResolution() const;
  void ResetOverscan(int res, OVERSCAN &overscan);
  void ResetOverscan(RESOLUTION_INFO &resinfo);
  void ResetScreenParameters(int res);
  void Lock() { EnterCriticalSection(*this);  }
  void Unlock() { LeaveCriticalSection(*this); }
  float GetPixelRatio(int iRes) const;
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear();
  void GetAllowedResolutions(std::vector<int> &res);

  // output scaling
  void SetRenderingResolution(int res, float posX, float posY, bool needsScaling);  ///< Sets scaling up for rendering
  void SetScalingResolution(int res, float posX, float posY, bool needsScaling);    ///< Sets scaling up for skin loading etc.
  float GetScalingPixelRatio() const;
  void Flip();
  void InvertFinalCoords(float &x, float &y) const;
  inline float ScaleFinalXCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformXCoord(x, y, 0); }
  inline float ScaleFinalYCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformYCoord(x, y, 0); }
  inline float ScaleFinalZCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformZCoord(x, y, 0); }
  inline void ScaleFinalCoords(float &x, float &y, float &z) const XBMC_FORCE_INLINE { m_finalTransform.TransformPosition(x, y, z); }
  bool RectIsAngled(float x1, float y1, float x2, float y2) const;

  inline float GetGUIScaleX() const XBMC_FORCE_INLINE { return m_guiScaleX; }
  inline float GetGUIScaleY() const XBMC_FORCE_INLINE { return m_guiScaleY; }
  inline color_t MergeAlpha(color_t color) const XBMC_FORCE_INLINE
  {
    color_t alpha = m_finalTransform.TransformAlpha((color >> 24) & 0xff);
    if (alpha > 255) alpha = 255;
    return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
  }

  void SetOrigin(float x, float y);
  void RestoreOrigin();
  void SetCameraPosition(const XbmcCPoint &camera);
  void RestoreCameraPosition();
  bool SetClipRegion(float x, float y, float w, float h);
  void RestoreClipRegion();
  void ApplyHardwareTransform();
  void RestoreHardwareTransform();
  void ClipRect(XbmcCRect &vertex, XbmcCRect &texture, XbmcCRect *diffuse = NULL);
  void ClipToViewWindow();
  inline void ResetWindowTransform()
  {
    while (m_groupTransform.size())
      m_groupTransform.pop();
    m_groupTransform.push(m_guiTransform);
  }
  inline void AddTransform(const TransformMatrix &matrix)
  {
    ASSERT(m_groupTransform.size());
    if (m_groupTransform.size())
      m_groupTransform.push(m_groupTransform.top() * matrix);
    else
      m_groupTransform.push(matrix);
    UpdateFinalTransform(m_groupTransform.top());
  }
  inline void RemoveTransform()
  {
    ASSERT(m_groupTransform.size() > 1);
    if (m_groupTransform.size())
      m_groupTransform.pop();
    if (m_groupTransform.size())
      UpdateFinalTransform(m_groupTransform.top());
    else
      UpdateFinalTransform(TransformMatrix());
  }

protected:
  void SetFullScreenViewWindow(int &res);

  std::stack<XbmcCRect> m_viewStack;

  int m_iScreenHeight;
  int m_iScreenWidth;
  int m_iScreenId;
  int m_iBackBufferCount;
  bool m_bWidescreen;
  CStdString m_strMediaDir;
  XbmcCRect m_videoRect;
  bool m_bFullScreenRoot;
  bool m_bFullScreenVideo;
  bool m_bCalibrating;
  int m_Resolution;

private:
  void UpdateCameraPosition(const XbmcCPoint &camera);
  void UpdateFinalTransform(const TransformMatrix &matrix);
  int m_windowResolution;
  float m_guiScaleX;
  float m_guiScaleY;
  std::stack<XbmcCPoint> m_cameras;
  std::stack<XbmcCPoint> m_origins;
  std::stack<XbmcCRect>  m_clipRegions;

  TransformMatrix m_guiTransform;
  TransformMatrix m_finalTransform;
  std::stack<TransformMatrix> m_groupTransform;
};

/*!
 \ingroup graphics
 \brief
 */
extern CGraphicContext g_graphicsContext;
#endif
