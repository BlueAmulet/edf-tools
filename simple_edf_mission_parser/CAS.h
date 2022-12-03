#pragma once
#include "include/tinyxml2.h"

struct CASTControl
{
	int pos;
	std::wstring wstr;
	int offset;
	std::vector< char > bytes;
	bool hasanm;
};

struct CASVControl
{
	int pos;
	std::wstring wstr;
	std::vector< char > bytes;
};

struct CASAnmGroup
{
	int pos = 0;
	std::wstring wstr;
	std::vector< char > bytes;
};

class CAS
{
public:
	void Read(std::wstring path);
	void ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header);

	void ReadTControlData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadVControlData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadAnmGroupData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadAnmGroupNodeData(std::vector<char> buffer, int ptrpos, tinyxml2::XMLElement* xmlptr, int index);
	void ReadAnmGroupNodeDataPtr(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadAnmGroupNodeDataPtrB(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadAnmGroupNodeDataPtrCommon(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadAnmGroupNodeDataCommon(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	// to cas
	void Write(const std::wstring& path);
	std::vector< char > WriteData(tinyxml2::XMLElement* Data);

	CASTControl WriteTControlData(tinyxml2::XMLElement* data, std::vector< char >* bytes);
	CASVControl WriteVControlData(tinyxml2::XMLElement* data);
	std::vector< char > WriteUnknownData(tinyxml2::XMLElement* data);
	std::vector< char > WriteCASSpecialData(tinyxml2::XMLElement* data, int num);
	CASAnmGroup WriteAnimationGroupData(tinyxml2::XMLElement* data, int subnum);
	CASAnmGroup WriteAnimationSetData(tinyxml2::XMLElement* data, int subnum);
	std::vector< char > WriteMainAnimationDataA(tinyxml2::XMLElement* data);
	std::vector< char > WriteMainAnimationDataB(tinyxml2::XMLElement* data);

private:
	int CAS_Version;
	// CasDataCommon length
	int i_CasDCCount = 0;
	int CANM_Offset;
	int i_TControlCount = 0;
	int i_TControlOffset = 0;
	int i_VControlCount = 0;
	int i_VControlOffset = 0;
	int i_AnmGroupCount = 0;
	int i_AnmGroupOffset = 0;
	int i_BoneCount = 0;
	int i_BoneOffset = 0;
	int i_UnkCOffset = 0;

	std::vector< std::wstring > WBoneList;

	std::vector< CASTControl > v_TControl;
	std::vector< CASVControl > v_VControl;
	std::vector< CASAnmGroup > v_AnmGroup;
	std::vector< CASAnmGroup > v_AnmSet;
	std::vector< char > v_AnmSetData;

};