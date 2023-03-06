#pragma once

#include "Engine/Math/Vec3.hpp"

#include "Game/Entity/ActorUID.hpp"

#include <string>
#include <functional>


enum class BTDataType
{
    VOID,
    NUMBER,
    VECTOR,
    BOOLEAN,
    TEXT,
    POINTER,
    ACTOR,
};

namespace E_BTDataType
{
    const char* GetName(BTDataType value);
    BTDataType GetFromName(const char* name);
}

enum class LogicOp
{
    NOT, // !
    AND, // &&
    OR,  // ||
    EQ,  // ==
    NE,  // !=
    GE,  // >=
    GT,  // >
    LE,  // <=
    LT,  // <
};


// using Destructor = std::function<void(void*)>;
// 
// struct ObjectRef
// {
// public:
//     static ObjectRef emptyString;
// 
// private:
//     Destructor destructor;
// 
// public:
//     void *const object;
// 
// public:
//     template<class T>
//     ObjectRef(T* t)
//         : destructor([](void* ptr) { delete reinterpret_cast<T*>(t); })
//         , object(t)
//     {
//     }
// 
//     ~ObjectRef()
//     {
//         destructor(object);
//     }
// 
// };


struct Value
{

public:
    static inline Value VOID() { return Value(); }
    static inline Value TRUE() { return Value(true); };
    static inline Value FALSE() { return Value(false); };
    static inline Value ZERO() { return Value(0.0); };
    static inline Value ONE() { return Value(1.0); };
    static inline Value EMPTY() { return Value(""); };
    static inline Value VEC_ONE()    { return Value(1.0f, 0.0f, 0.0f); };
    static inline Value VEC_ZERO()   { return Value(0.0f, 0.0f, 0.0f); };
    static inline Value VEC_UINT_X() { return Value(1.0f, 0.0f, 0.0f); };
    static inline Value VEC_UINT_Y() { return Value(0.0f, 1.0f, 0.0f); };
    static inline Value VEC_UINT_Z() { return Value(0.0f, 0.0f, 1.0f); };
    static inline Value NULL_ACTOR() { return Value(ActorUID::INVALID()); }

    inline explicit Value(BTDataType type)
        : type(type)
    {
        Clear();
    }

    inline explicit Value()
        : type(BTDataType::VOID)
        , data()
    {
    }

    inline explicit Value(bool b)
        : type(BTDataType::BOOLEAN)
        , data(b)
    {
    }

    inline explicit Value(double num)
        : type(BTDataType::NUMBER)
        , data(num)
    {
    }

    inline explicit Value(Vec3 vec)
        : type(BTDataType::VECTOR)
        , data(vec)
    {
    }

    inline explicit Value(float x, float y, float z)
        : type(BTDataType::VECTOR)
        , data(Vec3(x, y, z))
    {
    }

    inline explicit Value(const std::string& text)
        : type(BTDataType::TEXT)
        , data(text)
    {
    }

    inline explicit Value(const char* text)
        : type(BTDataType::TEXT)
        , data(text)
    {
    }

    inline explicit Value(void* obj)
        : type(BTDataType::POINTER)
        , data(obj)
    {
    }

    inline explicit Value(ActorUID obj)
        : type(BTDataType::POINTER)
        , data(obj)
    {
    }

    inline Value(const Value& other)
        : type(other.type)
    {
        *this = other;
    }

    ~Value();

    inline void operator=(const Value& other)
    {
        switch (type)
        {
        case BTDataType::VOID:
            return;
        case BTDataType::BOOLEAN:
            data.boo = other.GetAsBool();
            return;
        case BTDataType::NUMBER:
            data.num = other.GetAsNumber();
            return;
        case BTDataType::VECTOR:
            data.vec = other.GetAsVector();
            return;
        case BTDataType::TEXT:
            data.text = other.GetAsText();
            return;
        case BTDataType::POINTER:
            data.ptr = other.GetAsObject<void>();
        case BTDataType::ACTOR:
            data.actor = other.GetAsActor();
        }
    }

    inline bool operator==(const Value& ref) const
    {
        if (ref.type != type)
            return false;
        switch (type)
        {
        case BTDataType::VOID:
            return true;
        case BTDataType::BOOLEAN:
            return data.boo == ref.data.boo;
        case BTDataType::NUMBER:
            return data.num == ref.data.num;
        case BTDataType::VECTOR:
            return data.vec == ref.data.vec;
        case BTDataType::TEXT:
            return data.text == ref.data.text;
        case BTDataType::POINTER:
            return data.ptr == ref.data.ptr;
        case BTDataType::ACTOR:
            return data.actor == ref.data.actor;
        }
    }

    inline bool operator!=(const Value& ref) const
    {
        return !operator==(ref);
    }

    inline bool GetAsBool() const
    {
        return type == BTDataType::BOOLEAN ? data.boo : false;
    }

    inline int GetAsInteger() const
    {
        return type == BTDataType::NUMBER ? (int)data.num : 0;
    }

    inline float GetAsFloat() const
    {
        return type == BTDataType::NUMBER ? (float) data.num : 0.0f;
    }

    inline double GetAsNumber() const
    {
        return type == BTDataType::NUMBER ? data.num : 0.0;
    }

    inline Vec3 GetAsVector() const
    {
        return type == BTDataType::VECTOR ? data.vec : Vec3::ZERO;
    }

    const std::string& GetAsText() const;

    template<typename T>
    T* GetAsObject() const
    {
        return type == BTDataType::POINTER ? (T*)data.ptr : nullptr;
    }

    inline ActorUID GetAsActor() const
    {
        return type == BTDataType::ACTOR ? data.actor : ActorUID::INVALID();
    }

    inline void Set(bool val)                        { if (type == BTDataType::BOOLEAN) data.boo = val; }
    inline void Set(int val)                         { if (type == BTDataType::NUMBER) data.num = (double) val; }
    inline void Set(float val)                       { if (type == BTDataType::NUMBER) data.num = (double) val; }
    inline void Set(double val)                      { if (type == BTDataType::NUMBER) data.num = (double) val; }
    inline void Set(Vec3 val)                        { if (type == BTDataType::VECTOR) data.vec = val; }
    inline void Set(float x, float y, float z)       { if (type == BTDataType::VECTOR) data.vec = Vec3(x, y, z); }
    inline void Set(const std::string& val)          { if (type == BTDataType::TEXT) data.text = val; }
    inline void Set(const char* val)                 { if (type == BTDataType::TEXT) data.text = val; }
    inline void Set(void* val)                       { if (type == BTDataType::POINTER) data.ptr = val; }
    inline void Set(ActorUID val)                    { if (type == BTDataType::ACTOR) data.actor = val; }

    inline void Clear()
    {
        switch (type)
        {
        case BTDataType::VOID:
            break;
        case BTDataType::BOOLEAN:
            data.boo = false;
            break;
        case BTDataType::NUMBER:
            data.num = 0;
            break;
        case BTDataType::VECTOR:
            data.vec = Vec3::ZERO;
            break;
        case BTDataType::TEXT:
            data.text = "";
            break;
        case BTDataType::POINTER:
            data.ptr = nullptr;
            break;
        case BTDataType::ACTOR:
            data.actor = ActorUID::INVALID();
            break;
        default:
            break;
        }
    }

    inline BTDataType GetType() const { return type; }

private:
    const BTDataType type;

    union Data
    {
        Data() : boo(false) {}
        Data(bool val) : boo(val) {}
        Data(double val) : num(val) {}
        Data(Vec3 val) : vec(val) {}
        Data(const std::string& val) : text(val) {}
        Data(const char* val) : text(val) {}
        Data(void* val) : ptr(val) {}
        Data(ActorUID val) : actor(val) {}

        ~Data() {}

        bool                boo;
        double              num;
        Vec3                vec;
        std::string         text;
        void*               ptr;
        ActorUID            actor;
    } data;

};

