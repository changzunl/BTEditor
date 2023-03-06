#pragma once

#include "Game/Editor/BTCommons.hpp"

#include <string>
#include <vector>
#include <functional>

class ByteBuffer;

using DataEntryHandle = int;
struct Field;
using FieldList = std::vector<Field>;

constexpr DataEntryHandle INVALID_DATAENTRY_HANDLE = -1;

struct DataEntry
{
public:
    DataEntryHandle index = 0;
    std::string name = "";
    BTDataType type = BTDataType::BOOLEAN;
};


class DataRegistry
{
public:
    static DataRegistry instance;

public:
    DataEntryHandle           RegisterEntry(const char* name, BTDataType type);
    DataEntryHandle           GetHandle(const char* name);
    const std::string&        GetName(DataEntryHandle handle);
    const DataEntry*          GetEntry(DataEntryHandle handle);

    void Load(ByteBuffer* buffer);
    void Save(ByteBuffer* buffer) const;
    void CollectProps(FieldList& fields);

private:
    std::string            m_name;
    std::vector<DataEntry> m_entries;
};


struct DataStorageEntry
{
    DataEntryHandle handle;
    Value value;
};


class DataTable
{
public:
    DataTable(DataRegistry& registry);

    DataStorageEntry* FindEntry(DataEntryHandle handle);
    DataStorageEntry* SetEntry(DataEntryHandle handle);
   void UnsetEntry(DataEntryHandle handle);

public:
    DataRegistry* const m_registry;

private:
    std::vector<DataStorageEntry> m_storage;
};


class Item
{
public:
    static DataEntryHandle ENTRY_ID;
    static DataEntryHandle ENTRY_NAME;

public:
    int m_weight = 0;

    static void SortByWeight();


    static std::vector<Item*> s_items;
};

class Filter
{
public:
    void SortBy(std::vector<Item*>& container, std::function<bool(const Item* a, const Item* b)> filter, bool inverse);
};

