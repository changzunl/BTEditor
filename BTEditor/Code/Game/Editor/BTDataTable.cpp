#include "Game/Editor/BTDataTable.hpp"

#include "Engine/Core/ByteBuffer.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Game/Editor/BTCommons.hpp"
#include "Game/Editor/UIGraph.hpp"

#include <algorithm>


DataRegistry DataRegistry::instance;

DataEntryHandle DataRegistry::RegisterEntry(const char* name, BTDataType type)
{
    if (m_entries.size() > 0xFF00ULL)
        return INVALID_DATAENTRY_HANDLE; // throw "registry full";

    for (auto& ref : m_entries)
        if (ref.name == name)
            return INVALID_DATAENTRY_HANDLE; // throw "duplicate entry";

    DataEntry entry;
    entry.index = (int)m_entries.size();
    entry.name = name;
    entry.type = type;
    m_entries.push_back(entry);

    return entry.index;
}

DataEntryHandle DataRegistry::GetHandle(const char* name)
{
    for (auto& ref : m_entries)
        if (ref.name == name)
            return ref.index;
    return INVALID_DATAENTRY_HANDLE;
}

const std::string& DataRegistry::GetName(DataEntryHandle handle)
{
    static std::string empty = "";
    auto entry = GetEntry(handle);
    return entry ? entry->name : empty;
}

const DataEntry* DataRegistry::GetEntry(DataEntryHandle handle)
{
    if (handle == INVALID_DATAENTRY_HANDLE)
        return nullptr;
    return &m_entries[handle];
}

void DataRegistry::Load(ByteBuffer* buffer)
{
    m_entries.clear();

    int flag;
    buffer->Read(flag);

    int version = (flag >> 24) & 0xFF;

    if (version > 0)
    {
        ByteUtils::ReadString(buffer, m_name);
    }
    else
    {
        m_name = "Board";
    }

    int size = flag & 0xFFFF;

    m_entries.reserve(size);

    DataEntry entry;
    for (int i = 0; i < size; i++)
    {
        buffer->Read(entry.index);
        buffer->Read(entry.type);
        ByteUtils::ReadString(buffer, entry.name);
        buffer->ReadAlignment();

        m_entries.push_back(entry);
    }
}

void DataRegistry::Save(ByteBuffer* buffer) const
{
    int flag = 0;
    int version = 1;
    flag |= (version & 0xFF) << 24;
    int size = (int)m_entries.size();
    flag |= size & 0xFFFF;

    buffer->Write(flag);

    ByteUtils::WriteString(buffer, m_name);

    for (auto& entry : m_entries)
    {
        buffer->Write(entry.index);
        buffer->Write(entry.type);
        ByteUtils::WriteString(buffer, entry.name);
    }
}

void DataRegistry::CollectProps(FieldList& fields)
{
    Field* f;

    fields.emplace_back();
    f = &fields.back();

    f->name = "BoardName";
    f->value = m_name;
    f->type = FieldType::TEXT;
    f->callback = [&](auto value)
    {
        return m_name = value;
    };

    for (auto& entry : m_entries)
    {
        fields.emplace_back();
        f = &fields.back();

        f->name = "Name";
        f->value = entry.name;
        f->type = FieldType::TEXT;
        f->callback = [&entry](auto value)
        {
            return entry.name = value;
        };

        fields.emplace_back();
        f = &fields.back();

        f->name = "Type";
        f->value = E_BTDataType::GetName(entry.type);
        f->defaults = {
        "VOID",
        "NUMBER",
        "VECTOR",
        "BOOLEAN",
        "TEXT",
        "POINTER",
        "ACTOR",
        };
        f->type = FieldType::TEXT;
        f->callback = [&entry](auto value)
        {
            
            return E_BTDataType::GetName(entry.type = E_BTDataType::GetFromName(value.c_str()));
        };
    }
}

DataEntryHandle Item::ENTRY_ID = DataRegistry::instance.RegisterEntry("id", BTDataType::NUMBER);
DataEntryHandle Item::ENTRY_NAME = DataRegistry::instance.RegisterEntry("name", BTDataType::TEXT);

void Item::SortByWeight()
{
    Filter filter;
    std::vector<Item*> items = s_items;
    auto sorter = [](const Item* a, const Item* b) { return a->m_weight >= b->m_weight; };
    filter.SortBy(items, sorter, false);
}

std::vector<Item*> Item::s_items;

void Filter::SortBy(std::vector<Item*>& container, std::function<bool(const Item* a, const Item* b)> filter, bool inverse)
{
    if (!inverse)
    {
        std::sort(container.begin(), container.end(), filter);
    }
    else
    {
        decltype(filter) filterInv = [filter](const Item* a, const Item* b) { return !filter(a, b); };
        std::sort(container.begin(), container.end(), filterInv);
    }
}

DataStorageEntry* DataTable::FindEntry(DataEntryHandle handle)
{
    for (auto& entry : m_storage)
        if (handle == entry.handle)
        {
            return &entry;
        }
    return nullptr;
}

DataStorageEntry* DataTable::SetEntry(DataEntryHandle handle)
{
    if (handle == INVALID_DATAENTRY_HANDLE)
        return nullptr;

    for (auto& entry : m_storage)
        if (handle == entry.handle)
        {
            return &entry;
        }

    m_storage.push_back(DataStorageEntry{ handle, Value(m_registry->GetEntry(handle)->type) });
    return &m_storage.back();
}

void DataTable::UnsetEntry(DataEntryHandle handle)
{
    for (auto ite = m_storage.begin(); ite != m_storage.end(); ite++)
    {
        if (ite->handle == handle)
        {
            ite = m_storage.erase(ite);
            return;
        }
    }
}

DataTable::DataTable(DataRegistry& registry)
    : m_registry(&registry)
{

}
