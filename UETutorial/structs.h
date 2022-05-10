#include <string>
#include <cstdint>
#include <Windows.h>
#include <locale>
#include <format>

struct FUObjectItem
{
	// Pointer to the allocated object
	struct UObject* Object;
	// Internal flags
	int Flags;
	// UObject Owner Cluster Index
	int ClusterRootIndex;
	// Weak Object Pointer Serial number associated with the object
	int SerialNumber;
};

class FFixedUObjectArray
{
public:
	/** Static master table to chunks of pointers **/
	FUObjectItem* Objects;
	/** Number of elements we currently have **/
	int MaxElements;
	/** Current number of UObject slots */
	int NumElements;

	FORCEINLINE UObject* GetObjectPtr(int Index)
	{
		if (Index >= 0 && Index < NumElements)
			return (&Objects[Index])->Object;

		return nullptr;
	}
};

void (*Free)(void*); // FMemory::Free

template <typename InElementType>
struct TArray
{
	InElementType* Data;
	int32_t Count; // ArrayNum
	int32_t Max; // ArrayMax

	void FreeArray()
	{
		Count = 0;
		Max = 0;
		Free(Data);
		Data = nullptr;
	}
};

class FString
{
private:
	typedef TArray<TCHAR> DataType;
	DataType Data;

public:
	void FreeString()
	{
		Data.FreeArray();
	}

	std::string ToString() const
	{
		auto Length = std::wcslen(Data.Data);
		std::string Result(Length, '\0');
		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data.Data, Data.Data + Length, '?', &Result[0]);
		return Result;
	}
};

void(__fastcall* ToStringO)(const struct FName*, FString&);

struct FName
{
	int	ComparisonIndex;
	unsigned int Number;

	std::string ToString() const
	{
		FString temp;
		
		ToStringO(this, temp);
		auto Str = temp.ToString();
		temp.FreeString();
		return Str;
	}
};

FFixedUObjectArray* ObjObjects;

struct UObject
{
	void** VFTable;
	int ObjectFlags;
	int InternalIndex;
	UObject* ClassPrivate;
	FName NamePrivate;
	UObject* OuterPrivate;

	auto GetName() const { return NamePrivate.ToString(); }
	
	auto GetFullName() const
	{
		std::string temp;

		for (auto outer = OuterPrivate; outer; outer = outer->OuterPrivate)
			temp = std::format("{}.{}", outer->GetName(), temp);

		return std::format("{} {}{}", ClassPrivate->GetName(), temp, this->GetName());
	}
};

UObject* FindObject(const std::string& objectName)
{
	for (int i = 0; i < ObjObjects->NumElements; i++)
	{
		auto Object = ObjObjects->GetObjectPtr(i);

		if (!Object) continue;

		auto ObjectName = Object->GetFullName();

		if (ObjectName.contains(objectName))
			return Object;
	}

	return nullptr;
}