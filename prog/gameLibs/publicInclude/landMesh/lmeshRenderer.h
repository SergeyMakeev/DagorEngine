//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <util/dag_stdint.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_color.h>
#include <math/dag_bounds3.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_texMgr.h>
#include <util/dag_simpleString.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <landMesh/lmeshTools.h>
#include <landMesh/landClass.h>
#include <shaders/dag_overrideStateId.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector.h>

enum LandClassData
{
  LC_TRIVIAL_DATA = 1,
  LC_GRASS_DATA = 2,
  LC_DETAIL_DATA = 4,
  LC_NORMAL_DATA = 8,
  LC_SWAP_VERTICAL_DETAIL = 16,
  LC_ALL_DATA = 0xFFFFFFFF
};
namespace landmesh
{
struct OptimizedScene;
}
struct LandClassDetailTextures;

class BaseTexture;
typedef BaseTexture Texture;
typedef BaseTexture ArrayTexture;
struct LCTexturesLoaded;

static constexpr int NUM_TEXTURES_STACK = 3;

struct CellState;
struct Frustum;
struct LandMeshCellDesc;
struct LandMeshCullingState;
struct LandMeshCullingData;
class LandMeshManager;
struct CellState;

#ifndef CELLPREPAREDCALLBACK_DEFINED // to prevent double declaration when including both lmeshRendererAces.h and
                                     // lmeshRendererClassic.h
#define CELLPREPAREDCALLBACK_DEFINED
class CellRegionCallback
{
public:
  virtual void startRenderCellRegion(int region) = 0;
  virtual void endRenderCellRegion(int region) = 0;
  virtual const IBBox2 *regions() const = 0;
  virtual int regionsCount() const = 0;
};
#endif


class LandMeshRenderer
{
public:
  enum
  {
    RENDER_DECALS = 1,
    RENDER_COMBINED = 2,
    RENDER_LANDMESH = 4,
    RENDER_HEIGHTMAP = 8
  };
  static uint32_t lmesh_render_flags;
  enum RenderPurpose
  {
    DEFAULT_RENDERING_PURPOSE,
    RENDER_FOR_GRASS
  };

  enum RenderType
  {
    RENDER_WITH_SPLATTING = 0,
    RENDER_CLIPMAP = 1,
    RENDER_GRASS_MASK = 2,
    MAX_RENDER_SPLATTING__ = RENDER_GRASS_MASK,
    RENDER_WITH_CLIPMAP = 3,
    RENDER_REFLECTION = 4,
    // RENDER_COMBINED_LAST  = 4,
    // MAX_WITH_CLIPMAP__ = RENDER_COMBINED_LAST,
    MAX_WITH_CLIPMAP__ = RENDER_REFLECTION,
    RENDER_DEPTH = 5,
    RENDER_ONE_SHADER = 6, // all shaders are the same! feedback, heightmap, vsm. render only landmesh and landmesh_combined
    RENDER_PATCHES = 7,
    RENDER_TYPES_COUNT
  };
  static constexpr int DET_TEX_NUM = 7;
  static constexpr int VER_LABEL = (DET_TEX_NUM << 8) | 2;

  friend struct LandMeshCullingState;

public:
  LandMeshRenderer(LandMeshManager &provider, dag::ConstSpan<LandClassDetailTextures> land_classes, TEXTUREID vert_tex_id,
    TEXTUREID vert_tex_nm_id, TEXTUREID vert_det_tex_id, TEXTUREID tile_tex, real tile_x, real tile_y);
  ~LandMeshRenderer();

  bool checkVerLabel() { return verLabel == VER_LABEL; }

  void prepare(LandMeshManager &provider, const Point3 &view_pos, float hmap_camera_height);
  // void set_land_classes(dag::ConstSpan<SimpleString> land_classes);

  void resetTextures();
  void setCustomLcTextures();

  void forceLowQuality(bool low_quality) { shouldForceLowQuality = low_quality; }
  bool forceTrivial(bool low_quality)
  {
    bool old = shouldRenderTrivially;
    shouldRenderTrivially = low_quality;
    return old;
  }
  void setUndetailedMicroDetail(float val) { undetailedLCMicroDetail = val; }  // default value is 0.971f
  void setIgnoreBottomDepthBias(bool value) { ignoreBottomDepthBias = value; } // default is -1e-6f

  void setRenderInBBox(const BBox3 &bbox) { renderInBBox = bbox; }
  void setUseHmapTankDetail(int lod) { useHmapTankDetail = lod; }
  int getUseHmapTankDetail() const { return useHmapTankDetail; }
  void setUseHmapTankSubDiv(int lod) { hmapSubDivLod0 = lod; }
  int getUseHmapTankSubDiv() const { return hmapSubDivLod0; }

  CellRegionCallback *setRegionCB(CellRegionCallback *cb)
  {
    CellRegionCallback *o = regionCallback;
    regionCallback = cb;
    return o;
  }

  void setMirroring(LandMeshManager &provider, int num_border_cells_x_pos, int num_border_cells_x_neg, int num_border_cells_z_pos,
    int num_border_cells_z_neg, float mirror_shrink_x_pos, float mirror_shrink_x_neg, float mirror_shrink_z_pos,
    float mirror_shrink_z_neg);

  void setCellsDebug(int dbg) { debugCells = dbg; }
  int getCustomLcCount();

#if _TARGET_PC
  static void afterLostDevice();
#endif
#if DAGOR_DBGLEVEL > 0
  static bool check_cull_matrix(const TMatrix &realView, const TMatrix4 &realProj, const TMatrix4 &realGlobtm, const char *marker,
    const LandMeshCullingData &data, bool do_fatal);
#endif

  void renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData, const TMatrix *realView,
    const TMatrix4 *realProj, const TMatrix4 *realGlobtm, const Point3 &view_pos, bool check_matrices = true,
    RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);
  void renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData, const Point3 &view_pos,
    RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);

  void render(LandMeshManager &provider, RenderType rtype, const Point3 &view_pos, RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);

  bool renderDecals(LandMeshManager &provider, RenderType rtype, const TMatrix4 &globtm, bool compatibility_mode);
  bool renderCulledDecals(LandMeshManager &provider, const LandMeshCullingData &culledData, bool compatibility_mode);

  static int lod1_switch_radius;
  ///!sets inverse (1/distance) distance to next geom LOD
  void setInvGeomLodDist(float invGeomLodDistance) { invGeomLodDist = invGeomLodDistance; }
  float getInvGeomLodDist() const { return invGeomLodDist; }
  void setMicrodetailScaleIfTrivial(float val) { detailedLCMicroDetail = val; }
  void evictSplattingData();
  TEXTUREID getDetailTileTex() { return tileTexId; }
  bool reloadGrassMaskTex(int land_class_id, TEXTUREID newGrassMaskTexId);
  const char *getTextureName(TEXTUREID tex_id);

  Tab<LandClassDetailTextures> &getLandClasses() { return landClasses; }

protected:
  __forceinline ShaderMesh *prepareGeomCellsLM(LandMeshManager &provider, int cellNo, int lodNo);
  __forceinline ShaderMesh *prepareGeomCellsCM(LandMeshManager &provider, int cellNo, bool **isBig);
  void renderGeomCellsLM(LandMeshManager &provider, uint16_t *cell_no, int cells_count, int lodNo, RenderType rtype,
    uint8_t use_exclusion);
  void renderGeomCellsCM(LandMeshManager &provider, uint16_t *cell_no, int cells_count, RenderType rtype, bool skip_not_big);

  void renderLandclasses(CellState &curState, bool useFilter = false, LandClassType lcFilter = LC_SIMPLE);
  struct MirroredCellState;
  bool renderCellDecals(LandMeshManager &provider, const MirroredCellState &mirroredCell);
  void renderCell(LandMeshManager &provider, int borderX, int borderY, RenderType rtype, RenderPurpose rpurpose,
    bool skip_combined_not_marked_as_big);

  void getCellState(LandMeshManager &provider, int cellX, int cellY, struct CellState &cell);

  int verLabel;
  TEXTUREID tileTexId;
  real tileXSize, tileYSize;
  IPoint2 centerCell;
  Point2 centerCellFract;       // centerPos-centerCellFract*cellSize
  float invGeomLodDist;         //<Distance to each next geom lod
  float landmeshCombinedDistSq; // squared distance to landmesh_combined
  bool shouldForceLowQuality, shouldRenderTrivially;
  int scaleVisRange;
  int hmapSubDivLod0;
  bool debugCells;
  enum HmapRenderType
  {
    NO_HMAP,
    TESSELATED_HMAP,
    ONEQUAD_HMAP
  };
  HmapRenderType renderHeightmapType;
  uint8_t useHmapTankDetail;
  CellState *cellStates;
  BBox3 renderInBBox;
  bbox3f frustumWorldBBox; // for render clipmap or grass mask

  float detMapTcScale, detMapTcOfs;

  int customLcCount;
  void prepareLandClasses(LandMeshManager &provider);
  Tab<LandClassDetailTextures> landClasses;
  Tab<LCTexturesLoaded> landClassesLoaded;
  carray<SmallTab<TEXTUREID, MidmemAlloc>, NUM_TEXTURES_STACK> megaDetails;
  carray<ArrayTexture *, NUM_TEXTURES_STACK> megaDetailsArray;
  // SmallTab<TEXTUREID, MidmemAlloc> grassMaskTexIds;
  TEXTUREID vertTexId;
  TEXTUREID vertNmTexId;
  TEXTUREID vertDetTexId;
  struct MirroredCellState
  {
    Point4 posToWorldSet[2];
    Color4 detMapTcSet;
    struct MirrorScaleState
    {
      uint8_t xz; // x+z*3
      // MirrorScaleState(uint32_t xz_):xz(xz_){}
      MirrorScaleState() : xz(0) {}
    };
    uint8_t invcull : 1;                 // x logical xor z
    uint8_t excluded : 1;                // x logical xor z
    static uint32_t current_mirror_mask; // 0 bit - x mirrored, 1 bit - z mirrored
    uint8_t cellX, cellY;                // from 0 to provider.getNumCellsX(), provider.getNumCellsY()

    static bool currentCullFlipped;
    static shaders::OverrideStateId currentCullFlippedPrevStateId;
    static shaders::OverrideStateId currentCullFlippedCurStateId;
    MirrorScaleState mirrorScaleState;

    bool getInvCull() const { return invcull; }

    void init(int borderX, int borderY, int mirrorX, int mirrorY, int x1, int y1, float cellSize, float gridCellSize,
      Point4 posToWorld[2], Color4 detMapTc, bool to_be_excluded);

    void setPosConsts(bool render_at_0 = false) const;
    void setDetMapTc() const;
    void setPsMirror() const;
    void setFlipCull(LandMeshRenderer *renderer) const;
    static void startRender();
  };
  SmallTab<MirroredCellState, MidmemAlloc> mirroredCells;
  int tWidth; // int tWidth = provider.getNumCellsX()+numBorderCellsXNeg+numBorderCellsXPos;


  int numBorderCellsXPos;
  int numBorderCellsXNeg;
  int numBorderCellsZPos;
  int numBorderCellsZNeg;
  float mirrorShrinkXPos;
  float mirrorShrinkXNeg;
  float mirrorShrinkZPos;
  float mirrorShrinkZNeg;
  CellRegionCallback *regionCallback;
  bool has_detailed_land_classes;
  Point2 tileScale;
  float detailedLCMicroDetail;   // scale microdetail IF land clasess are detailed, but landquality 0
  float undetailedLCMicroDetail; // scale microdetail otherwise (for compatibility tank mode we'd better have more tiled)
  Point4 worldMulPos[9][2];      // worldMulPos for all mirroring
  bool ignoreBottomDepthBias;
  landmesh::OptimizedScene *optScn;

  enum StateDepthBias
  {
    STATE_DEPTH_BIAS_ZERO,
    STATE_DEPTH_BIAS_BOTTOM
  };

  shaders::OverrideStateId setOverride(const shaders::OverrideState &new_state);
  void resetOverride(shaders::OverrideStateId &prev_state);
  shaders::OverrideStateId setStateFlipCull(bool flip_cull);
  shaders::OverrideStateId setStateDepthBias(StateDepthBias depth_bias);
  shaders::OverrideStateId setStateBlend();

  eastl::vector_map<uint32_t, eastl::vector<shaders::UniqueOverrideStateId>> overrideStateMap;
};

extern int big_clipmap_criterio;
