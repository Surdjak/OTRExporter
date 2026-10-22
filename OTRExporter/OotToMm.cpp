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
#undef FindResource

namespace OotToMm {

bool enabled = false;

uint16_t ActorId(uint16_t ootActorId) {
    if (!enabled) {
        return ootActorId;
    }
    return ootActorId;
}

uint16_t ObjectId(uint16_t ootObjectId) {
    if (!enabled) {
        return ootObjectId;
    }
    return ootObjectId;
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
                mm += (char)MM_SHIFT;
                mm += (char)arg[0];
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
