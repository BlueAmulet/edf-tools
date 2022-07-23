#pragma once

//Forward declare this
class CMissionScript;

//Function data map.
struct CMissionFnData
{

};

//Structure for a single mission function
struct MissionFunction
{
	MissionFunction::MissionFunction(){	};
	MissionFunction::MissionFunction( std::wstring srcCode, CMissionScript *script );

	std::wstring fnName;
	int argCount;
	bool doesReturn;

	//Compiler fns
	void Compile();
	void CompileLine( std::wstring fnStrn );
	void CompileArgs( std::wstring argsStrn );
	void CompileValue( std::wstring argsStrn );
	void CompileExpression( std::wstring argsStrn );

	//Decompiler fns
	void Decompile( );

	void GetName( );
	std::vector< char > GenerateFnDataBytes( int ofsToNameint, int fnPointerSize, int fnNum, int offset );

	//Used for compiler
	std::wstring sourceCode;
	std::vector< char > bytes;

	std::vector< int > patchBytePos;
	std::vector< std::wstring > patchFuncName;

	std::vector< int > patchConditionalPos;
	std::vector< int > elseConditionalPos;
	std::vector< int > whileStartPos;

	std::vector<std::wstring> ifArgs;

	//FnArgs and local vars.
	int m_iNumLocalVars = 0;
	std::vector<std::wstring> m_vecLocalVars;

	CMissionScript *myScript;
};

//Structure for the BVM header
struct BVMHeader
{
	BVMHeader::BVMHeader();
	BVMHeader::~BVMHeader();

	std::vector< char > GenerateBytes();

	std::string headerString;

	char *Padding1;
	char *Unknown1;
	char *Unknown2;

	int varArrSize;
	int varArrPtr;
	int fnArrSize;
	int fnArrPtr;

	int strnPtr;
};

class CMissionScript
{
public:
	//Decompiler
	void LoadLanguage( std::wstring path, int id );
	int Read( const std::wstring& path );
	void ReadFn( int position, std::vector<char> buffer, int numArgs, int nextFunctionStart );

	std::vector< int > m_vecFuncOffsets;
	std::vector< std::wstring > m_vecMissionStrns;

	void Preproccesser( std::wstring missionSourceRaw, std::wstring & missionSourceProccessed );

	//Compiler
	void Write( const std::wstring& path, int flags );

	int GetNumFunctions( ) { return m_vecFunctions.size( ); }
	int GetFnDataOfs( ) { return m_iFunctionDataOfs;  }
	std::wstring GetFnName( int index )	{ return m_vecFunctions[index]->fnName;	}

	int GetNumVars( ) { return m_vecVarNames.size( ); }
	std::wstring GetVarName( int index )	{ return m_vecVarNames[index]; }

private:
	int m_iStringBufferOfs;
	int m_iFunctionDataOfs;
	std::wofstream output;

	std::vector< std::wstring > m_vecVarNames;

	std::vector< std::wstring > m_vecReplaceStrings;

	//Compilation stuff
	BVMHeader *header;
	std::vector< MissionFunction * > m_vecFunctions;

	//Decompilation stuff:
	std::unique_ptr<CJSONAMLParser> languageFile[2];
};