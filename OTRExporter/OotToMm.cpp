#include "OotToMm.h"

#include <Globals.h>
#include <ZCutscene.h>
#include <ZPath.h>
#include <ZRoom/ZRoom.h>
#include <ZRoom/Commands/SetCameraSettings.h>
#include <ZRoom/Commands/SetCutscenes.h>
#include <ZRoom/Commands/SetRoomBehavior.h>
#include <Utils/MemoryStream.h>
#include "CutsceneExporter.h"
#include "DisplayListExporter.h"
#include "Main.h"
#include "../../hyrule/HyruleIds.h"
#undef FindResource

namespace OotToMm {

bool enabled = false;

namespace {
constexpr uint16_t OOT_ACTOR_PLAYER = 0x0000;
constexpr uint16_t OOT_ACTOR_EN_HOLL = 0x0023;
constexpr uint16_t MM_ACTOR_PLAYER = 0x0000;
constexpr uint16_t MM_ACTOR_EN_HOLL = 0x0018;
} // namespace

uint16_t ActorId(uint16_t ootActorId) {
    if (!enabled) {
        return ootActorId;
    }
    switch (ootActorId) {
        // Start positions spawn MM's Player.
        case OOT_ACTOR_PLAYER:
            return MM_ACTOR_PLAYER;
        // MM's room-loading planes stand in until OoT's are ported.
        case OOT_ACTOR_EN_HOLL:
            return MM_ACTOR_EN_HOLL;
        default:
            return ACTOR_OOT_BASE + ootActorId;
    }
}

uint16_t SpawnRotation(uint16_t ootAngle, uint8_t lowBits) {
    if (!enabled) {
        return ootAngle;
    }
    uint32_t degrees = ((uint32_t)ootAngle * 360 + 0x8000) / 0x10000 % 360;
    return (uint16_t)((degrees << 7) | (lowBits & 0x7F));
}

uint16_t TransitionParams(uint16_t ootActorId, uint16_t ootParams) {
    if (!enabled || ootActorId != OOT_ACTOR_EN_HOLL) {
        return ootParams;
    }
    // OoT keeps the type in bits 6-8 and a switch flag below; MM keeps the type in bits 7-9 and an
    // exit index below (MM fills in the transition actor index above them when spawning).
    // TODO: OoT's switch-flag hall (3) has no MM counterpart and becomes a plain visible one.
    static const uint8_t sMmTypes[8] = {
        /* ENHOLL_H_VISIBLE_NARROW */ 0,       /* ENHOLL_V_DOWN_BGCOVER_LARGE */ 3,
        /* ENHOLL_V_INVISIBLE */ 1,            /* ENHOLL_H_BGCOVER_SWITCHFLAG */ 0,
        /* ENHOLL_H_INVISIBLE */ 2,            /* ENHOLL_V_BGCOVER */ 3,
        /* ENHOLL_H_INVISIBLE_NARROW */ 2,     /* unused */ 0,
    };
    return (uint16_t)(sMmTypes[(ootParams >> 6) & 7] << 7);
}

uint16_t ObjectId(uint16_t ootObjectId) {
    if (!enabled) {
        return ootObjectId;
    }
    return OBJECT_OOT_BASE + ootObjectId;
}

int16_t CameraSetting(int16_t ootSetting) {
    if (!enabled) {
        return ootSetting;
    }
    // Matched by each setting's original name, given after the OoT name, and by what it does. Settings
    // MM has no counterpart for fall back to a general one and are marked TODO.
    static const int16_t sMmSettings[] = {
        /* 0x00 NONE                      */ 0x00, // NONE
        /* 0x01 NORMAL0                   */ 0x01, // NORMAL0
        /* 0x02 NORMAL1                   */ 0x0C, // NORMAL1
        /* 0x03 DUNGEON0                  */ 0x11, // DUNGEON0
        /* 0x04 DUNGEON1                  */ 0x1E, // DUNGEON1
        /* 0x05 NORMAL3                   */ 0x02, // NORMAL3
        /* 0x06 HORSE0                    */ 0x04, // HORSE
        /* 0x07 BOSS_GOMA                 */ 0x11, // TODO: OoT boss arenas use DUNGEON0
        /* 0x08 BOSS_DODO                 */ 0x11,
        /* 0x09 BOSS_BARI                 */ 0x11,
        /* 0x0A BOSS_FGANON               */ 0x11,
        /* 0x0B BOSS_BAL                  */ 0x11,
        /* 0x0C BOSS_SHADES               */ 0x11,
        /* 0x0D BOSS_MOFA                 */ 0x11,
        /* 0x0E TWIN0                     */ 0x11,
        /* 0x0F TWIN1                     */ 0x11,
        /* 0x10 BOSS_GANON1               */ 0x11,
        /* 0x11 BOSS_GANON2               */ 0x11,
        /* 0x12 TOWER0                    */ 0x28, // TOWER_ASCENT
        /* 0x13 TOWER1                    */ 0x28, // TOWER_ASCENT
        /* 0x14 FIXED0                    */ 0x0F, // FIXED0
        /* 0x15 FIXED1                    */ 0x1F, // FIXED1
        /* 0x16 CIRCLE0                   */ 0x0E, // CIRCLE0
        /* 0x17 CIRCLE2                   */ 0x24, // CIRCLE2
        /* 0x18 CIRCLE3                   */ 0x25, // CIRCLE3
        /* 0x19 PREREND0                  */ 0x06, // PREREND_FIXED
        /* 0x1A PREREND1                  */ 0x07, // PREREND_PIVOT
        /* 0x1B PREREND3                  */ 0x07, // TODO: MM has no side-scrolling prerendered camera
        /* 0x1C DOOR0                     */ 0x08, // DOORC
        /* 0x1D DOORC                     */ 0x08, // DOORC
        /* 0x1E RAIL3 (crawlspace)        */ 0x01, // TODO: MM has no crawlspace camera
        /* 0x1F START0                    */ 0x2C, // START0
        /* 0x20 START1                    */ 0x56, // START1
        /* 0x21 FREE0                     */ 0x0A, // FREE0
        /* 0x22 FREE2                     */ 0x0A, // FREE0
        /* 0x23 CIRCLE4                   */ 0x26, // CIRCLE4
        /* 0x24 CIRCLE5                   */ 0x03, // PIVOT_DIVING
        /* 0x25 DEMO0                     */ 0x09, // DEMO0
        /* 0x26 DEMO1                     */ 0x09, // DEMO0
        /* 0x27 MORI1                     */ 0x0B, // BIRDS_EYE_VIEW_0
        /* 0x28 ITEM0 (slow chest)        */ 0x1A, // LONG_CHEST_OPENING
        /* 0x29 ITEM1                     */ 0x12, // ITEM0
        /* 0x2A DEMO3                     */ 0x09, // DEMO0
        /* 0x2B DEMO4                     */ 0x1C, // ATTENTION
        /* 0x2C UFOBEAN                   */ 0x01, // TODO: MM has no magic bean ride camera
        /* 0x2D LIFTBEAN                  */ 0x01,
        /* 0x2E SCENE0                    */ 0x50, // SCENE0
        /* 0x2F SCENE1                    */ 0x50, // SCENE0
        /* 0x30 HIDAN1                    */ 0x11, // TODO: Fire and Forest Temple cameras use DUNGEON0
        /* 0x31 HIDAN2                    */ 0x11,
        /* 0x32 MORI2                     */ 0x11,
        /* 0x33 MORI3                     */ 0x11,
        /* 0x34 TAKO                      */ 0x37, // MINI_BOSS
        /* 0x35 SPOT05A                   */ 0x3C, // BIRDS_EYE_VIEW_1
        /* 0x36 SPOT05B                   */ 0x3C, // BIRDS_EYE_VIEW_1
        /* 0x37 HIDAN3                    */ 0x3C, // BIRDS_EYE_VIEW_1
        /* 0x38 ITEM2                     */ 0x14, // ITEM2
        /* 0x39 PIVOT_VERTICAL            */ 0x3B, // CIRCLE8, used for moving platforms
        /* 0x3A NORMAL2                   */ 0x4A, // NORMAL2
        /* 0x3B FISHING                   */ 0x01, // TODO: MM has no fishing pond camera
        /* 0x3C DEMOC                     */ 0x09, // DEMO0
        /* 0x3D UO_FIBER                  */ 0x11, // TODO: Jabu-Jabu tentacle rooms use DUNGEON0
        /* 0x3E DUNGEON2                  */ 0x32, // DUNGEON2
        /* 0x3F TEPPEN                    */ 0x01, // TODO: MM has no fixed-yaw camera
        /* 0x40 CIRCLE7                   */ 0x36, // CIRCLE7
        /* 0x41 NORMAL4                   */ 0x58, // NORMAL4
    };
    if (ootSetting < 0 || ootSetting >= (int16_t)(sizeof(sMmSettings) / sizeof(sMmSettings[0]))) {
        return ootSetting;
    }
    return sMmSettings[ootSetting];
}

void WriteRoomBehavior(BinaryWriter* writer, SetRoomBehavior* cmd) {
    // ZAPD splits the OoT command's second word into MM's fields for both games. OoT never sets the
    // point light and E2 bits. TODO: map OoT behavior types 1 and 2 onto MM's values.
    writer->Write(cmd->gameplayFlags);
    writer->Write(cmd->currRoomUnk2);
    writer->Write(cmd->currRoomUnk5);
    writer->Write(cmd->msgCtxUnk);
    writer->Write(cmd->enablePosLights);
    writer->Write(cmd->kankyoContextUnkE2);
}

void WriteMiscSettings(BinaryWriter* writer, SetCameraSettings* cmd) {
    // MM's 0x19 (SetWorldMapVisited) has no payload. TODO: carry OoT's scene camera type
    // (cmd->cameraMovement) and world map area (cmd->mapHighlight) somewhere MM's engine reads them.
    (void)writer;
    (void)cmd;
}

void WriteCutscenes(BinaryWriter* writer, ZResource* res, ZRoom* room, SetCutscenes* cmd) {
    // MM lists cutscenes in a custom 0x1F command; OoT points at a single cutscene. OoT starts scene
    // cutscenes from the entrance cutscene table rather than from this entry, so the exit, entrance
    // and flag fields are left unset. TODO: decide how OoT entrance cutscenes trigger in MM.
    writer->Seek(-4, SeekOffsetType::Current);
    writer->Write((uint32_t)RoomCommand::SetCutscenesMM);
    writer->Write((uint8_t)1);

    std::string listName;
    Globals::Instance->GetSegmentedPtrName(cmd->cmdArg2, room->parent, "CutsceneData", listName,
                                           res->parent->workerID);
    std::string fName = OTRExporter_DisplayList::GetPathToRes(room, listName);
    writer->Write(fName);
    writer->Write((uint16_t)0xFFFF);
    writer->Write((uint8_t)0xFF);
    writer->Write((uint8_t)0xFF);

    MemoryStream* csStream = new MemoryStream();
    BinaryWriter csWriter = BinaryWriter(csStream);
    OTRExporter_Cutscene cs;
    cs.Save(cmd->cutscenes[0], "", &csWriter);

    AddFile(fName, csStream->ToVector());
}

void WritePathExtras(BinaryWriter* writer, const PathwayEntry& path) {
    // MM paths add an additional path index and a custom value. TODO: confirm these "unused" values
    // against MM's own paths.
    (void)path;
    writer->Write((int8_t)-1);
    writer->Write((int16_t)-1);
}

namespace {
enum OotCode : uint8_t {
    OOT_NEWLINE = 0x01,
    OOT_END = 0x02,
    OOT_BOX_BREAK = 0x04,
    OOT_COLOR = 0x05,
    OOT_SHIFT = 0x06,
    OOT_TEXTID = 0x07,
    OOT_QUICKTEXT_ENABLE = 0x08,
    OOT_QUICKTEXT_DISABLE = 0x09,
    OOT_PERSISTENT = 0x0A,
    OOT_EVENT = 0x0B,
    OOT_BOX_BREAK_DELAYED = 0x0C,
    OOT_AWAIT_BUTTON_PRESS = 0x0D,
    OOT_FADE = 0x0E,
    OOT_NAME = 0x0F,
    OOT_OCARINA = 0x10,
    OOT_FADE2 = 0x11,
    OOT_SFX = 0x12,
    OOT_ITEM_ICON = 0x13,
    OOT_TEXT_SPEED = 0x14,
    OOT_BACKGROUND = 0x15,
    OOT_MARATHON_TIME = 0x16,
    OOT_RACE_TIME = 0x17,
    OOT_POINTS = 0x18,
    OOT_TOKENS = 0x19,
    OOT_UNSKIPPABLE = 0x1A,
    OOT_TWO_CHOICE = 0x1B,
    OOT_THREE_CHOICE = 0x1C,
    OOT_FISH_INFO = 0x1D,
    OOT_HIGHSCORE = 0x1E,
    OOT_TIME = 0x1F,
};

enum MmCode : uint8_t {
    MM_COLOR_DEFAULT = 0x00,
    MM_COLOR_RED = 0x01,
    MM_COLOR_GREEN = 0x02,
    MM_COLOR_BLUE = 0x03,
    MM_COLOR_YELLOW = 0x04,
    MM_COLOR_LIGHTBLUE = 0x05,
    MM_COLOR_PINK = 0x06,
    MM_TOKENS = 0x0D,
    MM_BOX_BREAK = 0x10,
    MM_NEWLINE = 0x11,
    MM_SHIFT = 0x14,
    MM_NAME = 0x16,
    MM_QUICKTEXT_ENABLE = 0x17,
    MM_QUICKTEXT_DISABLE = 0x18,
    MM_EVENT = 0x19,
    MM_PERSISTENT = 0x1A,
    MM_BOX_BREAK_DELAYED = 0x1B,
    MM_FADE = 0x1C,
    MM_SFX = 0x1E,
    MM_BTN_A = 0xB0,
    MM_CONTROL_PAD = 0xBB,
    MM_END = 0xBF,
    MM_TWO_CHOICE = 0xC2,
    MM_THREE_CHOICE = 0xC3,
    MM_TIME = 0xCA,
};

// Argument bytes that follow each OoT control code.
size_t OotArgCount(uint8_t code) {
    switch (code) {
        case OOT_COLOR:
        case OOT_SHIFT:
        case OOT_BOX_BREAK_DELAYED:
        case OOT_FADE:
        case OOT_ITEM_ICON:
        case OOT_TEXT_SPEED:
        case OOT_HIGHSCORE:
            return 1;
        case OOT_TEXTID:
        case OOT_FADE2:
        case OOT_SFX:
            return 2;
        case OOT_BACKGROUND:
            return 3;
        default:
            return 0;
    }
}

// OoT colors are 0x40 | index. Returns false for black, which MM lacks.
bool ConvertColor(uint8_t ootArg, uint8_t& mmCode) {
    static const int16_t table[] = { MM_COLOR_DEFAULT, MM_COLOR_RED,  MM_COLOR_GREEN,  MM_COLOR_BLUE,
                                     MM_COLOR_LIGHTBLUE, MM_COLOR_PINK, MM_COLOR_YELLOW, -1 };
    int16_t mm = table[ootArg & 7];
    if (mm < 0) {
        return false;
    }
    mmCode = (uint8_t)mm;
    return true;
}
} // namespace

Message ConvertMessage(const std::string& ootMsg, uint8_t textboxType, uint8_t textboxYPos,
                       std::vector<uint8_t>& unconverted) {
    // TODO: map OoT textbox types and positions onto MM's, which do not match one to one.
    Message out;
    out.textboxType = textboxType;
    out.textboxYPos = textboxYPos;

    std::string& mm = out.msg;
    size_t i = 0;
    while (i < ootMsg.size()) {
        uint8_t c = (uint8_t)ootMsg[i++];
        uint8_t arg[3] = {};
        size_t argCount = c < 0x20 ? OotArgCount(c) : 0;
        for (size_t k = 0; k < argCount && i < ootMsg.size(); k++) {
            arg[k] = (uint8_t)ootMsg[i++];
        }

        if (c >= 0x20 && c < 0x9F) {
            // ASCII and accented letters share their encoding. TODO: verify the 0x7F-0x9E range.
            mm += (char)c;
            continue;
        }
        if (c >= 0x9F && c <= 0xA9) {
            // Button glyphs A, B, C, L, R, Z, C-buttons and the Z-target arrow.
            mm += (char)(MM_BTN_A + (c - 0x9F));
            continue;
        }

        switch (c) {
            case OOT_END:
                mm += (char)MM_END;
                return out;
            case OOT_NEWLINE:
                mm += (char)MM_NEWLINE;
                break;
            case OOT_BOX_BREAK:
                mm += (char)MM_BOX_BREAK;
                break;
            case OOT_COLOR: {
                uint8_t mmColor;
                if (ConvertColor(arg[0], mmColor)) {
                    mm += (char)mmColor;
                } else {
                    mm += (char)MM_COLOR_DEFAULT;
                    unconverted.push_back(c);
                }
                break;
            }
            case OOT_SHIFT:
                // OoT shifts by pixels, MM by spaces, which are 6 pixels wide at MM's text scale.
                mm += (char)MM_SHIFT;
                mm += (char)((arg[0] + 3) / 6);
                break;
            case OOT_TEXTID:
                // TODO: confirm MM continues to nextMessageID the way OoT follows TEXTID.
                out.nextMessageID = (uint16_t)((arg[0] << 8) | arg[1]);
                break;
            case OOT_QUICKTEXT_ENABLE:
                mm += (char)MM_QUICKTEXT_ENABLE;
                break;
            case OOT_QUICKTEXT_DISABLE:
                mm += (char)MM_QUICKTEXT_DISABLE;
                break;
            case OOT_PERSISTENT:
                mm += (char)MM_PERSISTENT;
                break;
            case OOT_EVENT:
                mm += (char)MM_EVENT;
                break;
            case OOT_BOX_BREAK_DELAYED:
                mm += (char)MM_BOX_BREAK_DELAYED;
                mm += (char)0;
                mm += (char)arg[0];
                break;
            case OOT_FADE:
                mm += (char)MM_FADE;
                mm += (char)0;
                mm += (char)arg[0];
                break;
            case OOT_FADE2:
                mm += (char)MM_FADE;
                mm += (char)arg[0];
                mm += (char)arg[1];
                break;
            case OOT_NAME:
                mm += (char)MM_NAME;
                break;
            case OOT_SFX:
                // TODO: remap OoT sound effect ids to MM's.
                mm += (char)MM_SFX;
                mm += (char)arg[0];
                mm += (char)arg[1];
                break;
            case OOT_ITEM_ICON:
                // TODO: remap OoT item ids to the unified item table.
                out.icon = arg[0];
                break;
            case OOT_TOKENS:
                mm += (char)MM_TOKENS;
                break;
            case OOT_TWO_CHOICE:
                mm += (char)MM_TWO_CHOICE;
                break;
            case OOT_THREE_CHOICE:
                mm += (char)MM_THREE_CHOICE;
                break;
            case OOT_TIME:
                mm += (char)MM_TIME;
                break;
            case 0xAA:
                // OoT's control stick glyph; MM only has the control pad.
                mm += (char)MM_CONTROL_PAD;
                break;
            default:
                // Ocarina, timers, scores, fish info, text speed, backgrounds, "await button" and
                // anything else without an MM equivalent yet.
                unconverted.push_back(c);
                break;
        }
    }

    // A well-formed OoT message always ends in OOT_END.
    mm += (char)MM_END;
    return out;
}

uint32_t WriteCutsceneCommands(BinaryWriter* writer, ZCutscene* cs) {
    // TODO: translate OoT cutscene commands. Until then every command is skipped, leaving an MM
    // cutscene that only runs to its end frame.
    (void)writer;
    std::string skipped;
    for (const auto* cmd : cs->commands) {
        skipped += StringHelper::Sprintf(" 0x%X", cmd->commandID);
    }
    if (!skipped.empty()) {
        // Cutscenes reached through a room's cutscene command have no name of their own.
        printf("OotToMm: cutscene %s:0x%X not converted, skipped commands:%s\n", cs->parent->GetName().c_str(),
               (uint32_t)cs->GetRawDataIndex(), skipped.c_str());
    }
    return 0;
}

} // namespace OotToMm
