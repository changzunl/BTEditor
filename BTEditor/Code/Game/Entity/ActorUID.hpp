#pragma once

class Actor;
class World;


struct ActorUID
{
public:
	inline ActorUID() : m_data(0) {}
	inline ActorUID(int rawData) : m_data(rawData) {}
	inline ActorUID(int index, int salt) : m_data((salt << 16) | (index & 0xFFFF)) {}

	inline void Invalidate() { m_data = 0; }
	inline int  GetIndex() const { return m_data & 0xFFFF; }
	inline int GetRawData() const { return m_data; }
	inline bool operator==( const ActorUID& other ) const { return m_data == other.m_data; }
	inline bool operator!=( const ActorUID& other ) const { return m_data != other.m_data; }

    Actor* operator*() const;
    bool IsValid() const;

	inline static ActorUID INVALID() { return ActorUID(); }

private:
	static World* GetMap();

private:
	int m_data;
};

