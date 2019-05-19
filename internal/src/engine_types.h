#pragma once

#include <cstdint>

#define _pad(x,y) uint8_t x[y]

#pragma pack(push, 1)

struct WXString {
  /* 000h */ char* text;
  // Including null terminator
  /* 008h */ uint32_t buffer_length;
  /* 00Ch SIZE */
};

struct WXWideString {
  /* 000h */ wchar_t* text;
  // Including null terminator
  /* 008h */ uint32_t buffer_length;
  /* 00Ch SIZE */
};

template <typename T>
struct WXSortedListEntry {
  /* 000h */ uint32_t maybe_hash;
  /* 004h */ _pad(p004, 0x04);
  /* 008h */ T* value;
  /* 010h SIZE */
};

template <typename T>
struct WXSortedList {
  // Capacity is determined with an equivalent to msize
  /* 000h */ WXSortedListEntry<T>* entries;
  /* 008h */ uint32_t count;
  /* 00Ch */ uint8_t is_dirty;
  /* 00Dh */ _pad(p00D, 0x03);
  /* 010h SIZE */
};

struct WBundleDiskFile;

struct WDirectory {
  /* 000h */ void* vtable;
  /* 008h */ WXSortedList<WBundleDiskFile> files;
  /* 018h */ WXSortedList<WDirectory> children;
  /* 028h */ WDirectory* parent;
  /* 030h */ WXWideString name;
  /* 03Ch */ _pad(p03C, 0x14);
  /* 050h SIZE */
};

struct WBundleDiskFile {
  /* 000h */ void* vtable;
  /* 008h */ WDirectory* directory;
  /* 010h */ void* whatever;
  /* 018h */ WXWideString file_name;
  /* 024h */ _pad(p024, 0x1C);
  /* 040h */ uint32_t file_index;
  /* 044h */
};

struct WXVector3 {
  /* 000h */ float x;
  /* 004h */ float y;
  /* 008h */ float z;
  /* 00Ch SIZE */
};

struct WXVector2 {
  /* 000h */ float x;
  /* 004h */ float y;
  /* 008h SIZE */
};

template <typename T>
struct WXBuffer {
  /* 000h */ T* data;
  /* 008h */ uint32_t length;
  /* 00Ch SIZE */
};

struct WXParticleEmitterModuleData {
  // CParticleInitializerAlpha
  /* 000h */ WXBuffer<float> alpha;
  // CParticleInitializerColor
  /* 00Ch */ WXBuffer<WXVector3> color;
  // CParticleInitializerLifeTime
  /* 018h */ WXBuffer<float> lifetime;
  // CParticleInitializerPosition
  /* 024h */ WXBuffer<WXVector3> position;
  /* 030h */ uint32_t p030; // or float, my sample had 0 here so hard to tell
  // CParticleInitializerRotation
  /* 034h */ WXBuffer<float> rotation;
  // CParticleInitializerRotation3D
  /* 040h */ WXBuffer<WXVector3> rotation_3d;
  // CParticleInitializerRotationRate
  /* 04Ch */ WXBuffer<float> rotation_rate;
  // CParticleInitializerRotationRate3D
  /* 058h */ WXBuffer<WXVector3> rotation_rate_3d;
  // CParticleInitializerSize and CParticleInitializerSize3d
  /* 064h */ WXBuffer<WXVector2> size;
  /* 070h */ WXBuffer<WXVector3> size_orientation;
  /* 07Ch */ uint8_t size_keep_ratio;
  /* 07Dh */ _pad(p07D, 3);
  // CParticleInitializerSpawnBox and CParticleInitializerSpawnCircle
  /* 080h */ WXBuffer<WXVector3> spawn_extents;
  /* 08Ch */ WXBuffer<float> spawn_inner_radius;
  /* 098h */ WXBuffer<float> spawn_outer_radius;
  /* 0A4h */ uint8_t spawn_world_space;
  /* 0A5h */ uint8_t spawn_surface_only;
  /* 0A6h */ _pad(p0A6, 2);
  /* 0A8h */ WXVector3 p0A8;
  /* 0B4h */ _pad(p0B4, 12); // one big-ass pudding
  /* 0C0h */ float spawn_to_local_matrix[16];
  /* 100h */ uint8_t p100; // STARTING FROM HERE IS PARSED AFTER p218
  /* 101h */ uint8_t p101;
  /* 102h */ uint8_t p102;
  /* 103h */ uint8_t p103;
  /* 104h */ uint8_t p104;
  /* 105h */ uint8_t p105;
  /* 106h */ uint8_t p106; // CONTINUES FROM p230
  /* 107h */ _pad(p107, 1);
  /* 108h */ WXBuffer<WXVector3> velocity;
  /* 114h */ uint8_t velocity_world_space;
  /* 115h */ _pad(p115, 3);
  /* 118h */ WXBuffer<float> velocity_inherit_scale;
  /* 124h */ WXBuffer<float> velocity_spread_scale;
  /* 130h */ uint8_t velocity_spread_conserve_momentum;
  /* 131h */ _pad(p131, 3);
  /* 134h */ WXBuffer<float> p134;
  /* 140h */ uint32_t p140; // or float
  /* 144h */ uint32_t p144; // or float
  /* 148h */ uint32_t p148; // or float
  // CParticleModificatorVelocityOverLife (modulate defined by flags, absolute unspecified?)
  /* 14Ch */ WXBuffer<WXVector3> velocity_over_life;
  // CParticleModificatorAcceleration (world_space defined by flags)
  /* 158h */ WXBuffer<WXVector3> acceleration_direction;
  /* 164h */ WXBuffer<float> acceleration_scale;
  // CParticleModificatorRotationOverLife (modulate defined by flags)
  /* 170h */ WXBuffer<float> rotation_over_life;
  // CParticleModificatorRotationRateOverLife (modulate defined by flags)
  /* 17Ch */ WXBuffer<float> rotation_rate_over_life;
  // CParticleModificatorRotation3DOverLife (modulate defined by flags)
  /* 188h */ WXBuffer<WXVector3> rotation_3d_over_life;
  // CParticleModificatorRotationRate3DOverLife
  /* 194h */ WXBuffer<WXVector3> rotation_rate_3d_over_life;
  // CParticleModificatorColorOverLife (modulate defined by flags)
  /* 1A0h */ WXBuffer<WXVector3> color_over_life;
  // CParticleModificatorAlphaOverLife (modulate defined by flags)
  /* 1ACh */ WXBuffer<float> alpha_over_life;
  // CParticleModificatorSizeOverLife / CParticleModificatorSize3DOverLife (modulate defined by flags)
  /* 1B8h */ WXBuffer<WXVector2> size_over_life;
  // ... if drawer is CParticleDrawerEmitterOrientation (in compact, defined by flags)
  /* 1C4h */ WXBuffer<WXVector3> size_over_life_orientation;
  // CParticleModificatorTextureAnimation (initial frame is... missing? animation mode is defined by flags)
  /* 1D0h */ WXBuffer<float> texture_animation_speed;
  // CParticleModificatorVelocityTurbulize
  /* 1DCh */ WXBuffer<WXVector3> velocity_turbulize_scale;
  /* 1E8h */ WXBuffer<float> velocity_turbulize_timelife_limit;
  /* 1F4h */ float velocity_turbulize_noise_interval;
  /* 1F8h */ float velocity_turbulize_duration;
  // CParticleModificatorTarget and CParticleModificatorTargetNode (which is applied defined by flags)
  /* 1FCh */ WXBuffer<float> target_force_scale;
  /* 208h */ WXBuffer<float> target_kill_radius;
  /* 214h */ float target_max_force;
  /* 218h */ WXBuffer<WXVector3> target_position;
  /* 224h */ uint32_t p224; // or float
  /* 228h */ uint32_t p228; // should be integer as it is initialized to FFFFFFFF in constructor
  /* 22Ch */ _pad(p22C, 4);
  /* 230h */ uint64_t p230;
  /* 238h */ uint32_t p238; // or float
  /* 23Ch */ uint32_t p23C; // or float
  /* 240h */ uint32_t p240; // or float
  /* 244h */ uint32_t p244; // or float
  /* 248h */ uint8_t p248;
  /* 249h */ uint8_t p249;
  /* 24Ah */ _pad(p24A, 2);
  /* 24Ch */ uint32_t p24C; // or float
  /* 250h */ uint8_t p250;
  /* 251h */ _pad(p251, 3);
  /* 254h */ uint32_t collision_triggering_group_index;
  // CParticleModificatorAlphaByDistance
  /* 258h */ float alpha_by_distance_far;
  /* 25Ch */ float alpha_by_distance_near;
  /* 260h SIZE */
};

struct WRenderParticleEmitter {
  /* 000h */ _pad(p000, 0x10);
  /* 010h */ WXParticleEmitterModuleData emitter_data;
  /* 270h */ _pad(p270, 0x74);
  /* 2E4h */ uint32_t modificator_bitset;
  /* 2E8h */ _pad(p2E8, 0x0C);
  /* 2F4h */ uint32_t initializer_bitset;
  /* 2F8h */
};

struct WName {
  /* 000h */ uint32_t offset;
  /* 004h SIZE */
};

struct WBundleDataHandleReader {
  /* 000h */ void* vtable_one;
  /* 008h */ uint32_t hardcoded_0A;
  /* 00Ch */ uint32_t hardcoded_A3;
  /* 010h */ void* vtable_two;
  /* 018h */ void* p018;
  /* 020h */ WXBuffer<uint8_t>* buffer;
  /* 028h */ uint32_t read_cursor;
  /* 02Ch */ uint32_t read_limit;
  /* 030h */
};

struct WMemoryFileReader {
  /* 000h */ void* vtable_one;
  /* 008h */ uint32_t hardcoded_100052;
  /* 00Ch */ uint32_t hardcoded_A3;
  /* 010h */ void* vtable_two;
  /* 018h */ uint8_t* buffer;
  /* 020h */ uint64_t size;
  /* 028h */ uint64_t position;
  /* 030h SIZE */
};

struct WXBundleFileMapping {
  /* 000h */ _pad(p000, 0x10);
  /* 010h */ uint32_t file_id;
  /* 014h */ _pad(p014, 0x0C);
  /* 020h SIZE */
};

struct WXBitSet {
  /* 000h */ uint32_t* words;
  /* 008h */ uint32_t word_count;
  /* 00Ch */ uint32_t bit_count;
  /* 010h SIZE */
};

struct WXBundleFileLocation {
  /* 000h */ uint32_t file_index;
  /* 004h */ uint32_t bundle_index;
  /* 008h */ uint32_t p008;
  /* 00Ch */ uint32_t p00C;
  /* 010h */ uint32_t fallback_file_id_maybe;
  /* 014h SIZE */
};

struct WXBundleFileIndex {
  /* 000h */ _pad(p000, 0x30);
  /* 030h */ WXBundleFileMapping* mappings;
  /* 038h */ uint32_t mapping_count;
  /* 03Ch */ WXBundleFileLocation* locations;
};

struct WDiskBundle {
  /* 000h */ void* vtable;
  /* 008h */ WXString name;
  /* 014h */ WXString relative_path;
  /* 020h */ uint32_t index;
  /* 024h */ _pad(p024, 0x0C);
  /* 030h */ WXWideString absolute_path;
  /* 03Ch */
};

struct WXBundleManager {
  /* 000h */ _pad(p000, 0x10);
  /* 010h */ WXBundleFileIndex* file_index;
  /* 018h */ WXBitSet bundle_bitset_1;
  /* 028h */ WXBitSet bundle_bitset_2;
  /* 038h */ WXBitSet bundle_bitset_3;
  /* 048h */ _pad(p048, 0x08);
  /* 050h */ WDiskBundle** bundles;
  /* 058h */ int32_t bundle_count;
  /* 05Ch */ WBundleDiskFile** files;
  /* 064h */ int32_t file_count;
  /* 068h */
};

struct WDepot {
  /* 000h */ WDirectory base;
  /* 050h */ void* vtable_two;
  /* 058h */ _pad(p058, 0x100);
  /* 158h */ WXBundleManager* bundle_manager;
  /* 160h */
};

typedef void WIRTTIType;

struct WIReferencable {
  /* 000h */ void* vtable;
  /* 008h */ void* something;
  /* 010h SIZE */
};

struct WISerializable {

};

struct WProperty {
  /* 000h */ void* vtable;
  /* 008h */ WIRTTIType* rtti_type;
  /* 010h */ _pad(p008, 0x10);
  /* 020h */ uint32_t offset;
  /* 024h */ uint32_t flags;
  /* 028h */ _pad(p028, 0x08);
  /* 030h SIZE */
};

struct WParticleEmitter {
  /* 000h */ void* vtable_one;
  /* 008h */ _pad(p008, 0x110);
  /* 118h SIZE */
};

struct WDependencyLoader {
  /* 000h */ void* vtable_one;
  /* 008h */ _pad(p008, 0x10);
  /* 018h */ WBundleDiskFile* file;
  /* 020h */
};

#pragma pack(pop)
