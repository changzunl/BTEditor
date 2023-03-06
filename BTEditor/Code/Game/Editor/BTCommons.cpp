#include "Game/Editor/BTCommons.hpp"

const char* E_BTDataType::GetName(BTDataType value)
{
    static std::vector<std::string> names = {
        "VOID",
        "NUMBER",
        "VECTOR",
        "BOOLEAN",
        "TEXT",
        "POINTER",
        "ACTOR",
    };

    size_t index = (size_t) value;

    if (index >= names.size())
        index = 0;
    return names[index].c_str();
}

BTDataType E_BTDataType::GetFromName(const char* name)
{
    static std::vector<std::string> names = {
        "VOID",
        "NUMBER",
        "VECTOR",
        "BOOLEAN",
        "TEXT",
        "POINTER",
        "ACTOR",
    };

    for (int i = 0; i < names.size(); i++)
    {
        if (names[i] == name)
            return (BTDataType)i;
    }

    return BTDataType::VOID;
}

Value::~Value()
{
    if (type == BTDataType::POINTER)
    {

    }
}

const std::string& Value::GetAsText() const
{
    static std::string empty = "";
    return type == BTDataType::TEXT ? data.text : empty;
}

// ObjectRef ObjectRef::emptyString = ObjectRef(new std::string("abc"));
