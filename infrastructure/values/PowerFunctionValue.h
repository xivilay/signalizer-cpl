#ifndef CPL_POWERFUNCTIONVALUE_H
#define CPL_POWERFUNCTIONVALUE_H

#include "ValueBase.h"

namespace cpl
{
	class PowerSlopeValue : public ValueGroup
	{

	public:

		enum Index
		{
			Base, Pivot, Slope
		};

		struct PowerFunction
		{
			double a, b;
		};

		static constexpr double
			kMinDbs = -32,
			kMaxDbs = 32,
			kBaseMin = 2,
			kBaseMax = 10,
			kPivotMin = 10,
			kPivotMax = 20000;

		template<typename T>
		struct PowerSlopeSemantics
		{
		public:

			PowerSlopeSemantics()
				: dbRange(Math::dbToFraction(kMinDbs), Math::dbToFraction(kMaxDbs))
				, pivotRange(kPivotMin, kPivotMax)
				, baseRange(kBaseMin, kBaseMax)
			{


			}

			DBFormatter<T> dbFormatter;
			ExponentialRange<T> dbRange, pivotRange;
			LinearRange<T> baseRange;
			BasicFormatter<T> basicFormatter;
		};

		virtual std::size_t getNumValues() const noexcept override { return 3; }

		/// <summary>
		/// Safe and wait-free from any thread
		/// </summary>
		/// <returns></returns>
		virtual PowerFunction derive()
		{
			auto transform = [&](Index i) { return getValueIndex(i).getTransformer().transform(getValueIndex(i).getNormalizedValue()); };
			PowerFunction ret;
			ret.a = std::log(transform(Slope)) / std::log(transform(Base));
			ret.b = 1.0 / std::pow(transform(Pivot), ret.a);
			return ret;
		}
	};

	class CompletePowerSlopeValue : public PowerSlopeValue
	{
	public:
		CompletePowerSlopeValue()
			: base(&semantics.baseRange, &semantics.basicFormatter)
			, pivot(&semantics.pivotRange, &semantics.basicFormatter)
			, slope(&semantics.dbRange, &semantics.dbFormatter)
		{

		}

		ValueEntityBase & getValueIndex(std::size_t i) override { switch (i) { default: case 0: return base; case 1: return pivot; case 2: return slope; } }

	private:
		SelfcontainedValue<> base, pivot, slope;
		PowerSlopeSemantics<ValueT> semantics;
	};

	template<typename ParameterView>
	class ParameterPowerSlopeValue
		: public PowerSlopeValue
		, public cpl::Parameters::BundleUpdate<ParameterView>
	{
	public:

		typedef typename ParameterView::ValueType ValueType;
		typedef typename ParameterView::ParameterType ParameterType;
		typedef typename Parameters::BundleUpdate<ParameterView>::Record Entry;

		class SharedBehaviour : public PowerSlopeSemantics<ValueType>
		{
		public:
			const std::string & getContext() { return context; }

			std::string context = "PF.";
		};

		ParameterPowerSlopeValue(SharedBehaviour & b, std::string name = "")
			: base("Bs", b.baseRange, &b.basicFormatter)
			, pivot("Pvt", b.pivotRange, &b.basicFormatter)
			, slope("Slp", b.dbRange, &b.dbFormatter)
			, behaviour(b)
			, contextName(std::move(name))
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

		void generateInfo() override
		{
			parameters = std::make_unique<std::vector<Entry>>();
			auto add = [&](ParameterType & p) { parameters->push_back(Entry {&p, true, false}); };
			add(base); add(pivot); add(slope);
		}

		virtual std::string getContextualName() override { return contextName; }
		ValueEntityBase & getValueIndex(std::size_t i) override { return values[i]; }

		std::array<ParameterValueWrapper<ParameterView>, 3> values;
		ParameterType base, pivot, slope;

	private:
		std::unique_ptr<std::vector<Entry>> parameters;
		std::string contextName;
		SharedBehaviour & behaviour;
	};
};

#endif