#ifndef CPL_TRANSFORMVALUE_H
#define CPL_TRANSFORMVALUE_H

#include "Values.h"

namespace cpl
{

	class TransformValue : public ValueGroup
	{
	public:
		enum Index
		{
			X, Y, Z
		};

		enum Aspect
		{
			Position, Rotation, Scale
		};

		template<typename T>
		class SharedBehaviour : private BasicFormatter<T>
		{
		public:
			SharedBehaviour()
				: context("tsf")
				, degreeFormatter("degs")
				, degreeRange(0, 360)
				, magnitudeRange(0, 50)
			{

			}

			VirtualFormatter<T> & getDegreeFormatter() { return degreeFormatter; }
			VirtualFormatter<T> & getDefaultFormatter() { return *this; }
			VirtualTransformer<T> & getMagTransformer() { return magnitudeRange; }
			VirtualTransformer<T> & getDegreeTransformer() { return degreeRange; }
			const std::string & getContext() { return context; }

		private:
			std::string context;
			UnitFormatter<T> degreeFormatter;
			LinearRange<T> magnitudeRange, degreeRange;
		};

		virtual ValueEntityBase & getValueIndex(Aspect a, Index i) = 0;
		virtual ValueEntityBase & getValueIndex(std::size_t i) override
		{ 
			std::div_t res = std::div(i, 3); 
			return getValueIndex((Aspect)res.quot, (Index)res.rem);
		}

		virtual std::size_t getNumValues() const noexcept override { return 9; }
	};

	class CompleteTransformValue : TransformValue
	{
	public:
		CompleteTransformValue(SharedBehaviour<ValueT> & b)
			: vectors{
				{ b.getMagTransformer(), b.getDefaultFormatter() },
				{ b.getDegreeTransformer(), b.getDegreeFormatter() },
				{  b.getMagTransformer(), b.getDefaultFormatter() }
			}
		{

		}
	private:
		struct Vector
		{
			Vector(VirtualTransformer<ValueT> & tsf, VirtualFormatter<ValueT> & fmt)
				: axis{ { &tsf, &fmt }, { &tsf, &fmt }, { &tsf, &fmt } }
			{}
			SelfcontainedValue<> axis[3];
		};

		SharedBehaviour<ValueT> behaviour;
		virtual ValueEntityBase & getValueIndex(Aspect a, Index i) override { return vectors[a].axis[i]; }
		Vector vectors[3];
	};

	template<typename UIParameterView>
	class ParameterTransformValue
		: public TransformValue
		, public Parameters::BundleUpdate<UIParameterView>
	{

	public:
		typedef typename UIParameterView::ValueType ValueType;
		typedef typename UIParameterView::ParameterType ParameterType;
		typedef typename Parameters::BundleUpdate<UIParameterView>::Record Entry;

		ParameterTransformValue(SharedBehaviour<ValueType> & b)
			: vectors { 
				{ "pos.", b.getMagTransformer(), b.getDefaultFormatter() },
				{ "rot.", b.getDegreeTransformer(), b.getDegreeFormatter() },
				{ "scl.", b.getMagTransformer(), b.getDefaultFormatter() }
			}
			, behaviour(b)
		{

		}

		virtual const std::string & getBundleContext() const noexcept override
		{
			return behaviour.getContext();
		}

		virtual std::vector<Entry> & queryParameters() override
		{
			return *parameters.get();
		}

		virtual void parametersInstalled() override
		{
			for (std::size_t i = 0; i < values.size(); ++i)
					values[i].setParameterReference(parameters->at(i).uiParameterView);

			parameters = nullptr;
		}

		virtual void generateInfo() override
		{
			parameters = std::make_unique<std::vector<Entry>>();
			auto add = [&](ParameterType & p) { parameters->push_back(Entry{ &p, true, false }); };
			auto addV = [&](Vector & v) { add(v.axis[0]); add(v.axis[1]); add(v.axis[2]); };
			addV(vectors[0]); addV(vectors[1]); addV(vectors[2]);
		}

		ValueEntityBase & getValueIndex(Aspect a, Index i) override
		{
			return values[a * 3 + i];
		}

		struct Vector
		{
			Vector(const std::string & context, VirtualTransformer<ValueType> & tsf, VirtualFormatter<ValueType> & fmt)
				: axis{ {context + "x", tsf, &fmt}, { context + "y", tsf, &fmt }, { context + "z", tsf, &fmt } }
			{}
			ParameterType axis[3];
		};

		Vector vectors[3];
		std::array<ParameterValueWrapper<UIParameterView>, 9> values;

	private:
		std::unique_ptr<std::vector<Entry>> parameters;
		SharedBehaviour<ValueType> & behaviour;
	};
};

#endif