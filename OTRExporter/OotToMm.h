#pragma once

// Converts OoT data into MM's formats while OoT mode exports it (--convertToMm), so 2Ship can load
// Hyrule resources without OoT-specific importers. Everything here is a skeleton: formats are
// converted, but game semantics (IDs, flags, sound effects, cutscene commands) mostly are not yet.

#include <cstdint>
#include <string>
#include <vector>

#include <Utils/BinaryWriter.h>

class ZCutscene;
class ZResource;
class ZRoom;
class SetCameraSettings;
class SetCutscenes;
class SetRoomBehavior;
class PathwayEntry;

namespace OotToMm {

extern bool enabled;

// Scene

// These return their input unless enabled, so exporters can call them for either game.

// Maps actors MM already has an equivalent of (so far the room-loading planes). Other ids pass
// through. TODO: remap to the unified actor and object tables once OoT variants exist (suffixed _Oot).
uint16_t ActorId(uint16_t ootActorId);
uint16_t ObjectId(uint16_t ootObjectId);

// Spawn entries in MM store rotations as whole degrees in bits 7-15, with a cutscene id or half-day
// bits below. OoT stores binary angles. Used for start positions and transition actors.
// TODO: room actor lists still hold OoT angles, which some OoT actors use as extra parameters.
uint16_t SpawnRotation(uint16_t ootAngle, uint8_t lowBits);
// Converts a transition actor's parameters to its MM equivalent's layout.
uint16_t TransitionParams(uint16_t ootActorId, uint16_t ootParams);

// Maps an OoT camera setting (collision camera data) to MM's, whose list is ordered differently.
int16_t CameraSetting(int16_t ootSetting);

void WriteRoomBehavior(BinaryWriter* writer, SetRoomBehavior* cmd);
void WriteMiscSettings(BinaryWriter* writer, SetCameraSettings* cmd);
// Rewrites the command id already written for this command, so call it right after that id.
void WriteCutscenes(BinaryWriter* writer, ZResource* res, ZRoom* room, SetCutscenes* cmd);
void WritePathExtras(BinaryWriter* writer, const PathwayEntry& path);

// MM scene commands with no OoT counterpart are not emitted yet:
//   0x1A SetAnimatedMaterialList: OoT animates scene textures from code (z_scene_table.c).
//   0x1B SetActorCutsceneList:    OoT has no actor cutscene list.
//   0x1C SetMinimapList:          OoT minimap data lives in code tables.
//   0x1D Unused1D
//   0x1E SetMinimapChests:        OoT has no minimap chest markers.

// Text

struct Message {
    std::string msg;
    uint8_t textboxType;
    uint8_t textboxYPos;
    uint16_t icon = 0xFE;
    uint16_t nextMessageID = 0xFFFF;
    uint16_t firstItemCost = 0xFFFF;
    uint16_t secondItemCost = 0xFFFF;
};

// Translates an OoT message body (ending in 0x02) into an MM one (ending in 0xBF). Codes with no MM
// equivalent are dropped and their bytes are added to unconverted.
Message ConvertMessage(const std::string& ootMsg, uint8_t textboxType, uint8_t textboxYPos,
                       std::vector<uint8_t>& unconverted);

// Cutscenes

// Stub: writes the commands of an empty MM cutscene and returns how many it wrote.
uint32_t WriteCutsceneCommands(BinaryWriter* writer, ZCutscene* cs);

} // namespace OotToMm
