#include "TextExporter.h"
#include "../ZAPD/ZFile.h"
#include "OotToMm.h"
#include <map>

void OTRExporter_Text::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
	ZText* txt = (ZText*)res;

	if (OotToMm::enabled)
	{
		WriteHeader(txt, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::TSH_TextMM));

		writer->Write((uint32_t)txt->messages.size());

		std::map<uint8_t, size_t> unconvertedCounts;
		for (size_t i = 0; i < txt->messages.size(); i++)
		{
			std::vector<uint8_t> unconverted;
			OotToMm::Message msg = OotToMm::ConvertMessage(txt->messages[i].msg, txt->messages[i].textboxType,
			                                               txt->messages[i].textboxYPos, unconverted);
			for (uint8_t code : unconverted)
				unconvertedCounts[code]++;

			writer->Write(txt->messages[i].id);
			writer->Write(msg.textboxType);
			writer->Write(msg.textboxYPos);
			writer->Write(msg.icon);
			writer->Write(msg.nextMessageID);
			writer->Write(msg.firstItemCost);
			writer->Write(msg.secondItemCost);
			writer->Write(msg.msg);
		}

		for (const auto& [code, count] : unconvertedCounts)
			printf("OotToMm: %s dropped OoT text code 0x%02X %zu times\n", txt->GetName().c_str(), code, count);
		return;
	}

	WriteHeader(txt, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::SOH_Text));

	writer->Write((uint32_t)txt->messages.size());

	for (size_t i = 0; i < txt->messages.size(); i++)
	{
		writer->Write(txt->messages[i].id);
		writer->Write(txt->messages[i].textboxType);
		writer->Write(txt->messages[i].textboxYPos);
		writer->Write(txt->messages[i].msg);
	}
}
