#include <string>

class TaskNode;

class TaskNodePtr
{
public:
	TaskNode* operator*();

private:
	int m_nodeType = 0;

};


class TaskNode
{
public:


private:
	std::string name;
};
