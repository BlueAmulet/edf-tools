#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <sstream>

#include "util.h"
#include "CANM.h"
#include "CAS.h"
#include "include/tinyxml2.h"

void CAS::Read(std::wstring path)
{
	std::ifstream file(path + L".cas", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		// create xml
		tinyxml2::XMLDocument xml = new tinyxml2::XMLDocument();
		xml.InsertFirstChild(xml.NewDeclaration());
		tinyxml2::XMLElement* xmlHeader = xml.NewElement("CAS");
		xml.InsertEndChild(xmlHeader);

		ReadData(buffer, xmlHeader);
		
		std::string outfile = WideToUTF8(path) + "_CAS.xml";
		xml.SaveFile(outfile.c_str());
		/*
		tinyxml2::XMLPrinter printer;
		xml.Accept(&printer);
		auto xmlString = std::string{ printer.CStr() };

		std::wcout << UTF8ToWide(xmlString) + L"\n";
		*/
	}

	//Clear buffers
	buffer.clear();
	file.close();
}

void CAS::ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header)
{
	int position = 0;
	unsigned char seg[4];
	// read header, length is 0x30
	memcpy(&seg, &buffer[0], 4U);
	//if (seg[0] == 0x43 && seg[1] == 0x41 && seg[2] == 0x53 && seg[3] == 0x00)

	// read version
	memcpy(&CAS_Version, &buffer[4], 4U);
	if (CAS_Version == 512)
	{
		header->SetAttribute("version", "41");
		i_CasDCCount = 9;
	}
	else if (CAS_Version == 515)
	{
		header->SetAttribute("version", "5");
		i_CasDCCount = 13;
	}

	// read canm offset
	memcpy(&CANM_Offset, &buffer[8], 4U);

	// read t control
	memcpy(&i_TControlCount, &buffer[0x0C], 4U);
	memcpy(&i_TControlOffset, &buffer[0x10], 4U);
	// read v control
	memcpy(&i_VControlCount, &buffer[0x14], 4U);
	memcpy(&i_VControlOffset, &buffer[0x18], 4U);
	// read animation group
	memcpy(&i_AnmGroupCount, &buffer[0x1C], 4U);
	memcpy(&i_AnmGroupOffset, &buffer[0x20], 4U);
	// read bone
	memcpy(&i_BoneCount, &buffer[0x24], 4U);
	memcpy(&i_BoneOffset, &buffer[0x28], 4U);
	// read unk C
	memcpy(&i_UnkCOffset, &buffer[0x2C], 4U);

	// output CANM Data
	tinyxml2::XMLElement* xmlcanm = header->InsertNewChildElement("CanmData");
	std::vector<char> newbuf;
	// no data size, so copy remain
	int filesize = buffer.size() - CANM_Offset;
	for (int i = 0; i < filesize; i++)
		newbuf.push_back(buffer[CANM_Offset + i]);
	
	std::unique_ptr< CANM > CANMReader = std::make_unique< CANM >();
	CANMReader->ReadData(newbuf, xmlcanm);
	CANMReader.reset();

	// t control data
	ReadTControlData(header, buffer);
	// v control data
	ReadVControlData(header, buffer);
	// animation group data
	ReadAnmGroupData(header, buffer);
	// bone data
	ReadBoneListData(header, buffer);
	// unk c data
	tinyxml2::XMLElement* xmlunk = header->InsertNewChildElement("Unknown");
	if (i_UnkCOffset > 0)
		ReadAnmGroupNodeDataPtrCommon(buffer, xmlunk, i_UnkCOffset);
}

void CAS::ReadTControlData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlTCD = header->InsertNewChildElement("TControl");
	for (int i = 0; i < i_TControlCount; i++)
	{
		int curpos = i_TControlOffset + (i * 0xC);

		int value[3];
		memcpy(&value, &buffer[curpos], 12U);
		tinyxml2::XMLElement* xmlptr = xmlTCD->InsertNewChildElement("ptr");

		xmlptr->SetAttribute("index", i);
#if defined(DEBUGMODE)
		xmlptr->SetAttribute("pos", curpos);
#endif

		// get string
		std::wstring wstr;
		// str
		if (value[0] > 0)
			wstr = ReadUnicode(buffer, curpos + value[0]);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());

		// read number
		for (int j = 0; j < value[1]; j++)
		{
			int numpos = curpos + value[2] + (j * 4);

			int number;
			memcpy(&number, &buffer[numpos], 4U);

			tinyxml2::XMLElement* xmlNode = xmlptr->InsertNewChildElement("value");
			xmlNode->SetText(number);
		}
	}
	// end
}

void CAS::ReadVControlData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlun = header->InsertNewChildElement("VControl");
	for (int i = 0; i < i_VControlCount; i++)
	{
		int curpos = i_VControlOffset + (i * 0x14);

		int value[3];
		memcpy(&value, &buffer[curpos], 12U);
		tinyxml2::XMLElement* xmlptr = xmlun->InsertNewChildElement("ptr");

		xmlptr->SetAttribute("index", i);
#if defined(DEBUGMODE)
		xmlptr->SetAttribute("pos", curpos);
#endif

		// get string
		std::wstring wstr;
		// str
		if (value[0] > 0)
			wstr = ReadUnicode(buffer, curpos + value[0]);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());
		//
		xmlptr->SetAttribute("int1", value[1]);
		xmlptr->SetAttribute("int2", value[2]);

		float fv;
		memcpy(&fv, &buffer[curpos + 0xC], 4U);
		xmlptr->SetAttribute("float3", fv);

		int iv;
		memcpy(&iv, &buffer[curpos + 0x10], 4U);
		xmlptr->SetAttribute("int4", iv);
	}
}

void CAS::ReadAnmGroupData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlun = header->InsertNewChildElement("AnmGroup");
	for (int i = 0; i < i_AnmGroupCount; i++)
	{
		int curpos = i_AnmGroupOffset + (i * 0xC);

		int value[3];
		memcpy(&value, &buffer[curpos], 12U);
		tinyxml2::XMLElement* xmlptr = xmlun->InsertNewChildElement("ptr");

		xmlptr->SetAttribute("index", i);
#if defined(DEBUGMODE)
		xmlptr->SetAttribute("pos", curpos);
#endif

		// get string
		std::wstring wstr;
		// str
		if (value[0] > 0)
			wstr = ReadUnicode(buffer, curpos + value[0]);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());
		// get data
		for (int j = 0; j < value[1]; j++)
		{
			int ptrpos = curpos + value[2] + (j * 0x24);
			ReadAnmGroupNodeData(buffer, ptrpos, xmlptr, j);
		}
	}
}

void CAS::ReadAnmGroupNodeData(std::vector<char> buffer, int ptrpos, tinyxml2::XMLElement* xmlptr, int index)
{
	int ptrvalue[9];
	memcpy(&ptrvalue, &buffer[ptrpos], 36U);
	tinyxml2::XMLElement* xmlnode = xmlptr->InsertNewChildElement("node");

	xmlnode->SetAttribute("index", index);
#if defined(DEBUGMODE)
	xmlnode->SetAttribute("pos", ptrpos);
#endif

	// i0 is string offset
	std::wstring wstr;
	if (ptrvalue[0] > 0)
		wstr = ReadUnicode(buffer, ptrpos + ptrvalue[0]);
	else
		wstr = L"";
	std::string utf8str = WideToUTF8(wstr);
	xmlnode->SetAttribute("name", utf8str.c_str());
	// i1 is data1 offset
	tinyxml2::XMLElement* xmldata1 = xmlnode->InsertNewChildElement("data1");
	// Type A is now unavailable
	ReadAnmGroupNodeDataPtrB(buffer, xmldata1, ptrpos + ptrvalue[1]);
	// i2 is data2 amount, i3 is offset
	tinyxml2::XMLElement* xmldata2 = xmlnode->InsertNewChildElement("data2");
	for (int i = 0; i < ptrvalue[2]; i++)
	{
		int data2pos = ptrpos + ptrvalue[3] + (i*0x20);
		tinyxml2::XMLElement* xmldataptr = xmldata2->InsertNewChildElement("ptr");
		ReadAnmGroupNodeDataPtrB(buffer, xmldataptr, data2pos);
	}
	// i4, i5, i6 is offset
	tinyxml2::XMLElement* xmlptr1 = xmlnode->InsertNewChildElement("parametric1");
	if (ptrvalue[4] > 0)
		ReadAnmGroupNodeDataPtrCommon(buffer, xmlptr1, ptrpos + ptrvalue[4]);
	tinyxml2::XMLElement* xmlptr2 = xmlnode->InsertNewChildElement("parametric2");
	if (ptrvalue[5] > 0)
		ReadAnmGroupNodeDataPtrCommon(buffer, xmlptr2, ptrpos + ptrvalue[5]);
	tinyxml2::XMLElement* xmlptr3 = xmlnode->InsertNewChildElement("parametric3");
	if (ptrvalue[6] > 0)
		ReadAnmGroupNodeDataPtrCommon(buffer, xmlptr3, ptrpos + ptrvalue[6]);
	// check type
	xmlnode->SetAttribute("type", ptrvalue[7]);
	if (ptrvalue[7] == 0)
		xmlnode->SetAttribute("value", IntHexAsFloat(ptrvalue[8]));
	else if (ptrvalue[7] == 2)
		xmlnode->SetAttribute("value", ptrvalue[8]);
	else
		std::wcout << L"Unknown type at position: " + ToString(ptrpos + 0x1C) + L" - Type: " + ToString(ptrvalue[7]) + L"\n";
}

void CAS::ReadAnmGroupNodeDataPtrA(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos)
{
	// not using it now
	/*
	union test
	{
		int i;
		float f;
	} value[7];
	*/
	int value[7];
	memcpy(&value, &buffer[pos], 28U);
	// simple type check
	for (int i = 0; i < 7; i++)
	{
		tinyxml2::XMLElement* datanode;
		if (abs(value[i]) < 0x1000)
		{
			datanode = xmldata->InsertNewChildElement("int");
			datanode->SetText(value[i]);
		}
		else
		{
			datanode = xmldata->InsertNewChildElement("float");
			datanode->SetText(IntHexAsFloat(value[i]));
		}
	}
#if defined(DEBUGMODE)
	xmldata->SetAttribute("debugpos", pos);
#endif
}

void CAS::ReadAnmGroupNodeDataPtrB(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos)
{
	// not using it now
	/*
	union test
	{
		int i;
		float f;
	} value[7];
	*/
	int value[8];
	memcpy(&value, &buffer[pos], 32U);
	// simple type check
	for (int i = 0; i < 8; i++)
	{
		tinyxml2::XMLElement* datanode;
		if (abs(value[i]) < 0x1000)
		{
			datanode = xmldata->InsertNewChildElement("int");
			datanode->SetText(value[i]);
		}
		else
		{
			datanode = xmldata->InsertNewChildElement("float");
			datanode->SetText(IntHexAsFloat(value[i]));
		}
	}
#if defined(DEBUGMODE)
	xmldata->SetAttribute("debugpos", pos);
#endif
}

void CAS::ReadAnmGroupNodeDataPtrCommon(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos)
{
	int value[2];
	memcpy(&value, &buffer[pos], 8U);
	
	for (int i = 0; i < value[0]; i++)
	{
		int datapos = pos + value[1] + (i * i_CasDCCount * 4);
		tinyxml2::XMLElement* xmlptr = xmldata->InsertNewChildElement("data");
#if defined(DEBUGMODE)
		xmlptr->SetAttribute("debugpos", datapos);
#endif
		ReadAnmGroupNodeDataCommon(buffer, xmlptr, datapos);
	}
}

void CAS::ReadAnmGroupNodeDataCommon(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos)
{
	std::vector< int > value(i_CasDCCount);
	memcpy(&value[0], &buffer[pos], i_CasDCCount * 4);
	// simple type check
	for (int i = 0; i < i_CasDCCount; i++)
	{
		tinyxml2::XMLElement* datanode;
		if (abs(value[i]) < 0x1000)
		{
			datanode = xmldata->InsertNewChildElement("int");
			datanode->SetText(value[i]);
		}
		else
		{
			datanode = xmldata->InsertNewChildElement("float");
			datanode->SetText(IntHexAsFloat(value[i]));
		}
	}
}

void CAS::ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlbone = header->InsertNewChildElement("BoneList");
	for (int i = 0; i < i_BoneCount; i++)
	{
		int curpos = i_BoneOffset + (i * 4);

		int boneofs;
		memcpy(&boneofs, &buffer[curpos], 4U);
		tinyxml2::XMLElement* xmlptr = xmlbone->InsertNewChildElement("value");

#if defined(DEBUGMODE)
		xmlptr->SetAttribute("index", i);
		xmlptr->SetAttribute("pos", curpos);
#endif

		// get string
		std::wstring wstr;
		// str
		if (boneofs > 0)
			wstr = ReadUnicode(buffer, curpos + boneofs);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetText(utf8str.c_str());
	}
}

// to cas
void CAS::Write(const std::wstring& path)
{
	std::wstring sourcePath = path + L"_cas.xml";
	std::wcout << "Will output CAS file.\n";
	std::string UTF8Path = WideToUTF8(sourcePath);

	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLElement* header = doc.FirstChildElement("CAS");

	std::vector< char > bytes;
	bytes = WriteData(header);

	//Final write.
	/**/
	std::ofstream newFile(path + L".cas", std::ios::binary | std::ios::out | std::ios::ate);

	for (size_t i = 0; i < bytes.size(); i++)
	{
		newFile << bytes[i];
	}

	newFile.close();
	
	std::wcout << L"Conversion completed: " + path + L".cas\n";
}

std::vector<char> CAS::WriteData(tinyxml2::XMLElement* Data)
{
	std::vector< char > bytes, aibytes;
	tinyxml2::XMLElement* entry, * entry2, * entry3;

	// check version
	int Version = Data->IntAttribute("version");
	if (Version == 41)
	{
		CAS_Version = 512;
		i_CasDCCount = 9;
	}
	else if (Version == 5)
	{
		CAS_Version = 515;
		i_CasDCCount = 13;
	}


	// read TControl data
	entry = Data->FirstChildElement("TControl");
	for (entry2 = entry->FirstChildElement("ptr"); entry2 != 0; entry2 = entry2->NextSiblingElement("ptr"))
	{
		// number is written at the same time
		v_TControl.push_back(WriteTControlData(entry2, &aibytes));
	}
	// write number offset
	i_TControlCount = v_TControl.size();
	int size_TControl = i_TControlCount * 0xC;
	for (int i = 0; i < i_TControlCount; i++)
	{
		if (v_TControl[i].hasanm)
		{
			int offset = v_TControl[i].offset + size_TControl - v_TControl[i].pos;
			memcpy(&v_TControl[i].bytes[0x8], &offset, 4U);
		}
	}

	// read VControl data
	entry = Data->FirstChildElement("VControl");
	for (entry2 = entry->FirstChildElement("ptr"); entry2 != 0; entry2 = entry2->NextSiblingElement("ptr"))
	{
		v_VControl.push_back(WriteVControlData(entry2));
	}
	i_VControlCount = v_VControl.size();

	// read bone list data
	std::string bstr;
	entry = Data->FirstChildElement("BoneList");
	for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
	{
		bstr = entry2->GetText();
		WBoneList.push_back(UTF8ToWide(bstr));
	}
	i_BoneCount = WBoneList.size();
	std::vector< char > blbytes(i_BoneCount * 4);

	// read unknown data
	entry = Data->FirstChildElement("Unknown");
	std::vector< char > unkbytes = WriteUnknownData(entry);

	// preread animation group data
	int pr_anmNum = 0;
	int pr_anmPtrNum = 0;
	entry = Data->FirstChildElement("AnmGroup");
	for (entry2 = entry->FirstChildElement("ptr"); entry2 != 0; entry2 = entry2->NextSiblingElement("ptr"))
	{
		pr_anmNum++;
		for (entry3 = entry2->FirstChildElement("node"); entry3 != 0; entry3 = entry3->NextSiblingElement("node"))
			pr_anmPtrNum++;
	}
	i_AnmGroupCount = pr_anmNum;
	// read animation group data
	for (entry2 = entry->FirstChildElement("ptr"); entry2 != 0; entry2 = entry2->NextSiblingElement("ptr"))
	{
		v_AnmGroup.push_back(WriteAnimationGroupData(entry2, pr_anmPtrNum));
	}

	// get canm data
	entry = Data->FirstChildElement("CanmData");
	std::unique_ptr< CANM > writer = std::make_unique< CANM >();
	std::vector< char > canmbytes = writer->WriteData(entry);
	writer.reset();



	// generate header
	bytes.resize(0x30, 0);
	bytes[0] = 0x43;
	bytes[1] = 0x41;
	bytes[2] = 0x53;
	bytes[3] = 0x00;
	// set version
	memcpy(&bytes[4], &CAS_Version, 4U);

	// write TControl data
	i_TControlOffset = bytes.size();
	for (size_t i = 0; i < v_TControl.size(); i++)
	{
		// need to save this location
		v_TControl[i].pos = bytes.size();
		for (size_t j = 0; j < v_TControl[i].bytes.size(); j++)
			bytes.push_back(v_TControl[i].bytes[j]);
	}
	// write number data
	for (size_t i = 0; i < aibytes.size(); i++)
		bytes.push_back(aibytes[i]);
	memcpy(&bytes[0x0C], &i_TControlCount, 4U);
	memcpy(&bytes[0x10], &i_TControlOffset, 4U);

	// write VControl data
	i_VControlOffset = bytes.size();
	for (size_t i = 0; i < v_VControl.size(); i++)
	{
		// need to save this location
		v_VControl[i].pos = bytes.size();
		for (size_t j = 0; j < v_VControl[i].bytes.size(); j++)
			bytes.push_back(v_VControl[i].bytes[j]);
	}
	memcpy(&bytes[0x14], &i_VControlCount, 4U);
	memcpy(&bytes[0x18], &i_VControlOffset, 4U);

	// write bone list data (all 0)
	i_BoneOffset = bytes.size();
	for (size_t i = 0; i < blbytes.size(); i++)
		bytes.push_back(blbytes[i]);
	memcpy(&bytes[0x24], &i_BoneCount, 4U);
	memcpy(&bytes[0x28], &i_BoneOffset, 4U);
	// write unknown data
	i_UnkCOffset = bytes.size();
	for (size_t i = 0; i < unkbytes.size(); i++)
		bytes.push_back(unkbytes[i]);
	memcpy(&bytes[0x2C], &i_UnkCOffset, 4U);

	// write animation group data
	i_AnmGroupOffset = bytes.size();
	for (size_t i = 0; i < v_AnmGroup.size(); i++)
	{
		// need to save this location
		v_AnmGroup[i].pos = bytes.size();
		for (size_t j = 0; j < v_AnmGroup[i].bytes.size(); j++)
			bytes.push_back(v_AnmGroup[i].bytes[j]);
	}
	memcpy(&bytes[0x1C], &i_AnmGroupCount, 4U);
	memcpy(&bytes[0x20], &i_AnmGroupOffset, 4U);
	// write animation set data
	for (size_t i = 0; i < v_AnmSet.size(); i++)
	{
		// need to save this location
		v_AnmSet[i].pos = bytes.size();
		for (size_t j = 0; j < v_AnmSet[i].bytes.size(); j++)
			bytes.push_back(v_AnmSet[i].bytes[j]);
	}
	// write animation other data
	for (size_t i = 0; i < v_AnmSetData.size(); i++)
		bytes.push_back(v_AnmSetData[i]);

	// 16-byte alignment is required
	int i_Alignment = bytes.size() % 16;
	if (i_Alignment > 0)
	{
		for (int i = i_Alignment; i < 16; i++)
			bytes.push_back(0);
	}
	// write canm data
	CANM_Offset = bytes.size();
	for (size_t i = 0; i < canmbytes.size(); i++)
		bytes.push_back(canmbytes[i]);
	memcpy(&bytes[0x8], &CANM_Offset, 4U);

	// write TControl string
	for (size_t i = 0; i < v_TControl.size(); i++)
	{
		int curpos = bytes.size() - v_TControl[i].pos;
		memcpy(&bytes[v_TControl[i].pos], &curpos, 4U);
		PushWStringToVector(v_TControl[i].wstr, &bytes);
	}
	// write VControl string
	for (size_t i = 0; i < v_VControl.size(); i++)
	{
		int curpos = bytes.size() - v_VControl[i].pos;
		memcpy(&bytes[v_VControl[i].pos], &curpos, 4U);
		PushWStringToVector(v_VControl[i].wstr, &bytes);
	}
	// write bone list string
	for (size_t i = 0; i < WBoneList.size(); i++)
	{
		int curpos = i_BoneOffset + (i * 4);
		int strpos = bytes.size() - curpos;
		memcpy(&bytes[curpos], &strpos, 4U);
		PushWStringToVector(WBoneList[i], &bytes);
	}
	// write animation group string
	for (size_t i = 0; i < v_AnmGroup.size(); i++)
	{
		int curpos = bytes.size() - v_AnmGroup[i].pos;
		memcpy(&bytes[v_AnmGroup[i].pos], &curpos, 4U);
		PushWStringToVector(v_AnmGroup[i].wstr, &bytes);
	}
	// write animation set string
	for (size_t i = 0; i < v_AnmSet.size(); i++)
	{
		int curpos = bytes.size() - v_AnmSet[i].pos;
		memcpy(&bytes[v_AnmSet[i].pos], &curpos, 4U);
		PushWStringToVector(v_AnmSet[i].wstr, &bytes);
	}

	return bytes;
}

CASTControl CAS::WriteTControlData(tinyxml2::XMLElement* data, std::vector<char>* bytes)
{
	CASTControl out;

	out.pos = v_TControl.size() * 0xC;
	out.offset = bytes->size();
	out.wstr = UTF8ToWide(data->Attribute("name"));
	out.hasanm = false;
	// write number
	int count = 0;
	tinyxml2::XMLElement* entry = data->FirstChildElement("value");
	if (entry != nullptr)
	{
		int number;
		char buffer[4];

		for (entry = data->FirstChildElement("value"); entry != 0; entry = entry->NextSiblingElement("value"))
		{
			number = entry->IntText();
			count++;

			memcpy(&buffer, &number, 4U);
			for (int i = 0; i < 4; i++)
				bytes->push_back(buffer[i]);
		}

		out.hasanm = true;
	}
	// - 0x04 - : Int32, Number amount.
	out.bytes.resize(0xC, 0);
	memcpy(&out.bytes[4], &count, 4U);

	return out;
}

CASVControl CAS::WriteVControlData(tinyxml2::XMLElement* data)
{
	CASVControl out;

	out.pos = v_VControl.size() * 0x14;
	out.wstr = UTF8ToWide(data->Attribute("name"));
	out.bytes.resize(0x14);

	int ig[2];
	ig[0] = data->IntAttribute("int1");
	ig[1] = data->IntAttribute("int2");
	memcpy(&out.bytes[4], &ig, 8U);

	float fvalue = data->FloatAttribute("float3");
	memcpy(&out.bytes[0xC], &fvalue, 4U);

	int ivalue = data->IntAttribute("int4");
	memcpy(&out.bytes[0x10], &ivalue, 4U);

	return out;
}

std::vector<char> CAS::WriteUnknownData(tinyxml2::XMLElement* data)
{
	std::vector<char> out(8, 0);
	// always offset 8
	out[4] = 8;
	// get count
	int count = 0;
	tinyxml2::XMLElement* entry = data->FirstChildElement("data");
	if (entry != nullptr)
	{
		for (entry = data->FirstChildElement("data"); entry != 0; entry = entry->NextSiblingElement("data"))
		{
			std::vector<char> buffer = WriteCASSpecialData(entry, i_CasDCCount);
			count++;

			for (size_t i = 0; i < buffer.size(); i++)
				out.push_back(buffer[i]);
		}
	}
	memcpy(&out[0], &count, 4U);

	return out;
}

std::vector<char> CAS::WriteCASSpecialData(tinyxml2::XMLElement* data, int num)
{
	std::vector<char> out;

	char buffer[4];
	tinyxml2::XMLElement* entry;
	std::string nodeType;
	for (entry = data->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
	{
		nodeType = entry->Name();
		if (nodeType == "int")
		{
			int value = entry->IntText();
			memcpy(&buffer, &value, 4U);
		}
		else if (nodeType == "float")
		{
			float value = entry->FloatText();
			memcpy(&buffer, &value, 4U);
		}
		// push byte
		for (int i = 0; i < 4; i++)
			out.push_back(buffer[i]);
	}
	// expand size, redundant fill 0
	out.resize(num * 4, 0);
	return out;
}

CASAnmGroup CAS::WriteAnimationGroupData(tinyxml2::XMLElement* data, int subnum)
{
	CASAnmGroup out;
	out.wstr = UTF8ToWide(data->Attribute("name"));
	out.bytes.resize(0xC, 0);
	// read set
	int count = 0;
	int offset = ((i_AnmGroupCount - v_AnmGroup.size()) * 0xC) + (v_AnmSet.size() * 0x24);
	for (tinyxml2::XMLElement* entry = data->FirstChildElement("node"); entry != 0; entry = entry->NextSiblingElement("node"))
	{
		v_AnmSet.push_back(WriteAnimationSetData(entry, subnum));
		count++;
	}

	memcpy(&out.bytes[4], &count, 4U);
	memcpy(&out.bytes[8], &offset, 4U);

	return out;
}

CASAnmGroup CAS::WriteAnimationSetData(tinyxml2::XMLElement* data, int subnum)
{
	CASAnmGroup out;
	out.wstr = UTF8ToWide(data->Attribute("name"));
	out.bytes.resize(0x24, 0);
	// other set
	int cusofs = (subnum - v_AnmSet.size()) * 0x24;
	tinyxml2::XMLElement* entry;
	// read type
	int type = data->IntAttribute("type");
	memcpy(&out.bytes[0x1C], &type, 4U);
	if (type == 0)
	{
		float value = data->FloatAttribute("value");
		memcpy(&out.bytes[0x20], &value, 4U);
	}
	else if (type == 2)
	{
		int value = data->IntAttribute("value");
		memcpy(&out.bytes[0x20], &value, 4U);
	}
	// read data1
	entry = data->FirstChildElement("data1");
	if (entry->FirstChildElement())
	{
		int value = cusofs + v_AnmSetData.size();
		memcpy(&out.bytes[4], &value, 4U);

		std::vector<char> buffer = WriteCASSpecialData(entry, 8);
		for (size_t i = 0; i < buffer.size(); i++)
			v_AnmSetData.push_back(buffer[i]);
	}
	// read data2
	entry = data->FirstChildElement("data2");
	if (entry->FirstChildElement())
	{
		int value = cusofs + v_AnmSetData.size();
		memcpy(&out.bytes[0xC], &value, 4U);

		value = 0;
		for (tinyxml2::XMLElement* entry2 = entry->FirstChildElement("ptr"); entry2 != 0; entry2 = entry2->NextSiblingElement("ptr"))
		{
			value++;
			std::vector<char> buffer = WriteCASSpecialData(entry2, 8);
			for (size_t i = 0; i < buffer.size(); i++)
				v_AnmSetData.push_back(buffer[i]);
		}
		memcpy(&out.bytes[0x8], &value, 4U);
	}
	// read parametric1
	entry = data->FirstChildElement("parametric1");
	if (entry->FirstChildElement())
	{
		int value = cusofs + v_AnmSetData.size();
		memcpy(&out.bytes[0x10], &value, 4U);

		std::vector<char> buffer = WriteUnknownData(entry);
		for (size_t i = 0; i < buffer.size(); i++)
			v_AnmSetData.push_back(buffer[i]);
	}
	// read parametric2
	entry = data->FirstChildElement("parametric2");
	if (entry->FirstChildElement())
	{
		int value = cusofs + v_AnmSetData.size();
		memcpy(&out.bytes[0x14], &value, 4U);

		std::vector<char> buffer = WriteUnknownData(entry);
		for (size_t i = 0; i < buffer.size(); i++)
			v_AnmSetData.push_back(buffer[i]);
	}
	// read parametric3
	entry = data->FirstChildElement("parametric3");
	if (entry->FirstChildElement())
	{
		int value = cusofs + v_AnmSetData.size();
		memcpy(&out.bytes[0x18], &value, 4U);

		std::vector<char> buffer = WriteUnknownData(entry);
		for (size_t i = 0; i < buffer.size(); i++)
			v_AnmSetData.push_back(buffer[i]);
	}

	return out;
}
