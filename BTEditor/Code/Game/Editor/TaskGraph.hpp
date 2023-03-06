#include "Game/Editor/TaskNode.hpp"

#include "Game/Editor/BTCommons.hpp"

#include <vector>
#include <functional>
#include <algorithm>

namespace TaskAST
{

    typedef void (*Destructor)(void*);

    struct ObjectRef
    {
        Destructor destructor;
        void* object;



        virtual ~ObjectRef()
        {
            destructor(object);
            free(object);
        }
    };

	enum class StatementType
	{
		SEQ,
		IF,
		WHILE,
		SWITCH,
	};

	class Statement
	{
	public:
		virtual Value Execute() = 0;
		virtual BTDataType GetReturnType() = 0;
	};

	class Expression : public Statement
	{
	public:
		virtual Value Evaluate() = 0;

		virtual Value Execute() override
		{
			return Evaluate();
		}

		virtual BTDataType GetReturnType() override
		{
			return BTDataType::BOOLEAN;
		}
	};

	class StaticExpression : Expression
	{
	public:
		virtual Value Evaluate() override
		{
			return value;
		}

		virtual BTDataType GetReturnType() override
		{
			return value.GetType();
		}

	private:
		Value value;
	};

	class LogicExpression : Expression
	{

	public:
		virtual Value Evaluate() override
		{
			return EvaluateBool() ? Value::TRUE() : Value::FALSE();
		}
		
		bool EvaluateBool()
		{
			switch (op)
			{
			case LogicOp::NOT:
				return !lExpression->Evaluate().GetAsBool();
			case LogicOp::AND:
				return lExpression->Evaluate().GetAsBool() && rExpression->Evaluate().GetAsBool();
			case LogicOp::OR:
				return lExpression->Evaluate().GetAsBool() || rExpression->Evaluate().GetAsBool();
			case LogicOp::GT:
				return lExpression->Evaluate().GetAsNumber() > rExpression->Evaluate().GetAsNumber();
			case LogicOp::GE:
				return lExpression->Evaluate().GetAsNumber() >= rExpression->Evaluate().GetAsNumber();
			case LogicOp::LT:
				return lExpression->Evaluate().GetAsNumber() < rExpression->Evaluate().GetAsNumber();
			case LogicOp::LE:
				return lExpression->Evaluate().GetAsNumber() <= rExpression->Evaluate().GetAsNumber();
			case LogicOp::EQ:
				return lExpression->Evaluate() == rExpression->Evaluate();
			case LogicOp::NE:
				return lExpression->Evaluate() != rExpression->Evaluate();
			}
		}

	private:
		LogicOp op;
		Expression* lExpression;
		Expression* rExpression;
    };

	enum class MathOp
	{
		SIN,
		COS,
		TAN,
		CEIL,
		FLOOR,
		ANGLE,
		RADIANS,
	};

    class MathOpExpression : Expression
    {

    public:
        virtual Value Evaluate() override
        {
            return Value(EvaluateDouble());
        }

        double EvaluateDouble()
        {
			constexpr double PI = 3.141592653;

            switch (op)
            {
            case MathOp::SIN:
                return sin(expression->Evaluate().GetAsNumber());
            case MathOp::COS:
                return cos(expression->Evaluate().GetAsNumber());
            case MathOp::TAN:
                return sin(expression->Evaluate().GetAsNumber());
            case MathOp::CEIL:
                return ceil(expression->Evaluate().GetAsNumber());
            case MathOp::FLOOR:
                return floor(expression->Evaluate().GetAsNumber());
            case MathOp::ANGLE:
                return expression->Evaluate().GetAsNumber() / PI * 180.0;
            case MathOp::RADIANS:
                return expression->Evaluate().GetAsNumber() / 180.0 * PI;
            }
        }

    private:
        MathOp op;
        Expression* expression;
    };


	enum class MathBiOp
	{
		MIN,
		MAX,
		ATAN2,
	};


    class MathBiOpExpression : Expression
    {

    public:
        virtual Value Evaluate() override
        {
            return Value(EvaluateDouble());
        }

        double EvaluateDouble()
        {
            constexpr double PI = 3.141592653;

			double l = expression1->Evaluate().GetAsNumber();
			double r = expression2->Evaluate().GetAsNumber();


            switch (op)
            {
            case MathBiOp::MIN:
                return r < l ? r : l;
            case MathBiOp::MAX:
                return r > l ? r : l;
            case MathBiOp::ATAN2:
                return atan2(l, r);
            }
        }

    private:
        MathBiOp op;
        Expression* expression1;
        Expression* expression2;
    };


    enum class MathTriOp
    {
        CLAMP,
        LERP,
    };


    class MathTriOpExpression : Expression
    {

    public:
        virtual Value Evaluate() override
        {
            return Value(EvaluateDouble());
        }

        double EvaluateDouble()
        {
            constexpr double PI = 3.141592653;

            double v1 = expression1->Evaluate().GetAsNumber();
            double v2 = expression2->Evaluate().GetAsNumber();
            double v3 = expression3->Evaluate().GetAsNumber();

            switch (op)
            {
            case MathTriOp::CLAMP:
                return v1 < v2 ? v2 : v1 > v3 ? v3 : v1;
            case MathTriOp::LERP:
                return v1 + (v2 - v1) * v3;
            }
        }

    private:
        MathTriOp op;
        Expression* expression1;
        Expression* expression2;
        Expression* expression3;
    };


	class SeqStatement : public Statement
	{
	public:
		Value Execute() override
		{
			for (Statement* statement : statements)
			{
				statement->Execute();
			}
			return Value();
		}

		virtual BTDataType GetReturnType() override
		{
			return BTDataType::VOID;
		}

	private:
		std::vector<Statement*> statements;
	};

	class IfStatement : public Statement
	{
	public:
		Value Execute() override
		{
			if (condition->Evaluate().GetAsBool())
			{
				return mainBody->Execute();
			}
			else
			{
				return elseBody->Execute();
			}
		}

		virtual BTDataType GetReturnType() override
		{
			return mainBody->GetReturnType();
		}

	public:
		Expression* condition;
		Statement* mainBody;
		Statement* elseBody;
	};

	class WhileStatement : public Statement
	{
	public:
		Value Execute() override
		{
			while (condition->Evaluate().GetAsBool())
			{
				body->Execute();
			}
			return Value();
		}

		virtual BTDataType GetReturnType() override
		{
			return BTDataType::VOID;
		}

	private:
		Expression* condition;
		Statement* body;
	};

	class SwitchStatement : public Statement
	{
	public:
		Value Execute() override
		{
			Value val = eval->Evaluate();
			for (int i = 0; i < size; i++)
			{
				if (values[i] == val)
				{
					return bodies[i]->Execute();
				}
			}
		}

		virtual BTDataType GetReturnType() override
		{
			return defaultBody == nullptr ? bodies == nullptr ? BTDataType::VOID : bodies[0]->GetReturnType() : defaultBody->GetReturnType();
		}

	private:
		int size = 0;
		Value* values;
		Expression* eval;
		Statement** bodies;
		Statement* defaultBody;
	};
}


class TaskGraph
{

};

