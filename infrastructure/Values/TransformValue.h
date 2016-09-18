#ifndef CPL_TRANSFORMVALUE_H
#define CPL_TRANSFORMVALUE_H

#include "Values.h"
#include "../../rendering/graphics.h"

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
				: context("Tsf.")
				, degreeFormatter("degs")
				, degreeRange(0, 360)
				, magnitudeRange(-50, 50)
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

		/// <summary>
		/// Fills in the transform using virtual dispatch
		/// </summary>
		template<typename T>
		void fillTransform3D(GraphicsND::Transform3D<T> & transform)
		{
			for (int x = 0; x < 3; ++x)
				for (int y = 0; y < 3; ++y)
				{
					auto & element = getValueIndex((Aspect)x, (Index)y);
					transform.element(x, y) = static_cast<T>(element.getTransformer().transform(element.getNormalizedValue()));
				}
		}


		template<typename T>
		void setFromTransform3D(const GraphicsND::Transform3D<T> & transform)
		{
			for (int x = 0; x < 3; ++x)
				for (int y = 0; y < 3; ++y)
				{
					getValueIndex((Aspect)x, (Index)y).setTransformedValue(transform.element(x, y));
				}
		}

		virtual ValueEntityBase & getValueIndex(Aspect a, Index i) = 0;
		virtual ValueEntityBase & getValueIndex(std::size_t i) override
		{ 
			std::div_t res = std::div(static_cast<int>(i), 3); 
			return getValueIndex((Aspect)res.quot, (Index)res.rem);
		}

		virtual std::size_t getNumValues() const noexcept override { return 9; }
	};

	class CompleteTransformValue : public TransformValue
	{
	public:
		CompleteTransformValue()
			: vectors{
				{ behaviour.getMagTransformer(), behaviour.getDefaultFormatter() },
				{ behaviour.getDegreeTransformer(), behaviour.getDegreeFormatter() },
				{  behaviour.getMagTransformer(), behaviour.getDefaultFormatter() }
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

	template<typename ParameterView>
	class ParameterTransformValue
		: public TransformValue
		, public Parameters::BundleUpdate<ParameterView>
	{

	public:
		typedef typename ParameterView::ValueType ValueType;
		typedef typename ParameterView::ParameterType ParameterType;
		typedef typename Parameters::BundleUpdate<ParameterView>::Record Entry;

		ParameterTransformValue(SharedBehaviour<ValueType> & b)
			: vectors { 
				{ "Pos.", b.getMagTransformer(), b.getDefaultFormatter() },
				{ "Rot.", b.getDegreeTransformer(), b.getDegreeFormatter() },
				{ "Scl.", b.getMagTransformer(), b.getDefaultFormatter() }
			}
			, behaviour(b)
		{

		}

		/// <summary>
		/// Fills in the transform with no virtual dispatch
		/// </summary>
		template<typename T>
		void fillDirectTransform3D(GraphicsND::Transform3D<T> & transform)
		{
			for (int x = 0; x < 3; ++x)
				for (int y = 0; y < 3; ++y)
				{
					auto & element = vectors[x].axis[y];
					transform.element(x, y) = element.getTransformer().transform(element.getValue());
				}
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
				: axis{ {context + "X", tsf, &fmt}, { context + "Y", tsf, &fmt }, { context + "Z", tsf, &fmt } }
			{}
			ParameterType axis[3];
		};

		Vector vectors[3];
		std::array<ParameterValueWrapper<ParameterView>, 9> values;

	private:
		std::unique_ptr<std::vector<Entry>> parameters;
		SharedBehaviour<ValueType> & behaviour;
	};
};

#endif