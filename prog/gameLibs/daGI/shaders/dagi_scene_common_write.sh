include "dagi_scene_voxels_common.sh"
float4 scene_voxels_invalid_start;
float4 scene_voxels_invalid_width;

float4 scene_voxels_unwrapped_invalid_start;
float4 scene_voxels_unwrapped_invalid_end;

macro SSGE_SCENE_COMMON_INIT_WRITE(code)
  INIT_VOXELS(code)
  (code) {
    scene_voxels_unwrapped_invalid_start@f4 = scene_voxels_unwrapped_invalid_start;
    scene_voxels_unwrapped_invalid_end@f3 = (scene_voxels_unwrapped_invalid_start + scene_voxels_invalid_width);
  }
endmacro

macro SSGE_WRITE_VOXEL_DATA(code, color_reg, alpha_reg)
  ENABLE_ASSERT(code)
  (code) {
    voxelsColor@uav: register(color_reg) hlsl {
      #define VOXELSCOLOR(type) RWTexture3D<type> voxelsColor@uav;
    }
    voxelsAlpha@uav: register(alpha_reg) hlsl {
      #define VOXELSALPHA RWTexture3D<float> voxelsAlpha@uav;
    }
  }
  hlsl(code) {
    #include <dagi_voxels_consts.hlsli>
    #if SCENE_VOXELS_COLOR == SCENE_VOXELS_R11G11B10
      VOXELSCOLOR(float3);
      VOXELSALPHA;
    #else
      VOXELSCOLOR(float4);
    #endif
    #include <dagi_write_voxels.hlsl>
    void writeVoxelData(uint3 coord, uint cascade, float3 color, float alpha)
    {
##if gi_quality == only_ao
      alpha *= max(0.01, luminance(color));
      writeSceneVoxelAlpha(coord, cascade, alpha);
##else
      writeSceneVoxelData(coord, cascade, color, alpha);
##endif
    }
  }
endmacro

int ssge_scene_common_color_reg_no = 7;
int ssge_scene_common_alpha_reg_no = 6;

macro SSGE_SCENE_COMMON_WRITE(code)
  USE_VOXELS(code)
  SSGE_WRITE_VOXEL_DATA(code, ssge_scene_common_color_reg_no, ssge_scene_common_alpha_reg_no)

  hlsl(code) {
    uint getCurrentCascade() {return uint(scene_voxels_unwrapped_invalid_start.w);}
    float3 currentSceneVoxelCenter(float3 worldPos)
    {
      uint cascade = getCurrentCascade();
      return sceneCoordToWorldPos(sceneWorldPosToCoord(worldPos, cascade), cascade);
      //faster, but assumes origin
      //float3 voxelSize = getSceneVoxelSize(cascade);
      //return (floor(worldPos/voxelSize)+0.5)*voxelSize;
    }
    bool safeToWriteVoxelData(float3 worldPos, inout uint3 wrappedCoord)
    {
      uint cascade = getCurrentCascade();
      int3 unwrappedCoord = sceneWorldPosToCoord(worldPos, cascade);
      //float3 origin = getSceneVoxelOrigin(cascade), voxelSize = getSceneVoxelSize(cascade);
      //int3 originCoord = floor(origin/voxelSize + 0.5);//fixme: replace with one int origin, more precise and less operations per pixel
      //int3 minCoord = (int3(scene_voxels_invalid_start.xyz) - originCoord), maxCoord = (minCoord + int3(scene_voxels_invalid_width.xyz));
      if (!(any(unwrappedCoord.xzy < scene_voxels_unwrapped_invalid_start.xyz) || any(unwrappedCoord.xzy >= scene_voxels_unwrapped_invalid_end.xyz)))//safety
      {
        wrappedCoord = wrapSceneVoxelCoord(unwrappedCoord, cascade);
        return true;
      }
      return false;
    }
    void writeSceneVoxelDataSafe(float3 worldPos, float3 col, float alpha)
    {
      uint3 wrappedCoord;
      if (safeToWriteVoxelData(worldPos, wrappedCoord))
        writeVoxelData(wrappedCoord, getCurrentCascade(), col, alpha / SCENE_VOXELS_ALPHA_SCALE);
    }
    void writeSceneVoxelDataUnsafe(float3 worldPos, float3 col, float alpha)
    {
      uint cascade = getCurrentCascade();
      writeVoxelData(wrapSceneVoxelCoord(sceneWorldPosToCoord(worldPos, cascade), cascade), cascade, col, alpha / SCENE_VOXELS_ALPHA_SCALE);
    }
  }
endmacro

macro SSGE_SCENE_COMMON_INIT_WRITE_PS()
  SSGE_SCENE_COMMON_INIT_WRITE(ps)
endmacro

macro SSGE_SCENE_COMMON_WRITE_PS()
  SSGE_SCENE_COMMON_WRITE(ps)
endmacro