#ifndef CPL_VALUES_H
#define CPL_VALUES_H

#include "parameters/CustomFormatters.h"
#include "parameters/CustomTransforms.h"
#include "parameters/ParameterSystem.h"
#include "../gui/Tools.h"
#include "../Misc.h"
#include "../CSerializer.h"
#include <set>
#include <array>
#include <string>
#include <vector>
#include <memory>

namespace cpl
{

	typedef double ValueT;

	class ValueEntityBase 
		: public CSerializer::Serializable
	{
	public:

		class ValueEntityListener /* : public virtual DestructionNotifier */
		{
		public:
			/// <summary>
			/// THe sender value is optional (may be zero), but it can be used to detect if you recived
			/// the message from yourself - for instance.
			/// </summary>
			virtual void valueEntityChanged(ValueEntityListener * sender, ValueEntityBase * value) = 0;
			virtual ~ValueEntityListener() {};
		};

		virtual VirtualTransformer<ValueT> & getTransformer() = 0;
		virtual VirtualFormatter<ValueT> & getFormatter() = 0;
		virtual ValueT getNormalizedValue() = 0;
		virtual void setNormalizedValue(ValueT parameter) = 0;
		virtual std::string getContextualName() { return {}; };

		virtual void beginChangeGesture() {};
		virtual void endChangeGesture() {};
		virtual void addListener(ValueEntityListener * listener) = 0;
		virtual void removeListener(ValueEntityListener * listener) = 0;
		
		virtual void serialize(CSerializer::Archiver & ar, Version version) { ar << getNormalizedValue(); }
		virtual void deserialize(CSerializer::Builder & builder, Version version) 
		{
			ValueT value;
			builder >> value;
			setNormalizedValue(value);
		}

		virtual ~ValueEntityBase() {}
	};

	class ColourValue
	{
	public:
		enum Index
		{
			R, G, B, A
		};
#ifdef CPL_JUCE
		juce::Colour getAsJuceColour() {
			auto index = [&](Index i) { return static_cast<float>(getValueIndex(i).getNormalizedValue()); };
			return juce::Colour::fromFloatRGBA(index(R), index(G), index(B), index(A));
		}

		void setFromJuceColour(juce::Colour colour)
		{
			getValueIndex(R).setNormalizedValue(colour.getFloatRed());
			getValueIndex(G).setNormalizedValue(colour.getFloatGreen());
			getValueIndex(B).setNormalizedValue(colour.getFloatBlue());
			getValueIndex(A).setNormalizedValue(colour.getFloatAlpha());
		}
#endif
		template<std::size_t bits>
		typename std::enable_if<bits < 32, std::uint32_t>::type getIntValueFor(Index i)
		{
			auto cap = (1 << bits);
			auto fpoint = getValueIndex(i).getNormalizedValue();
			return fpoint == (ValueT)1.0 ? (cap - 1) : static_cast<std::uint32_t>(fpoint * cap);
		}

		virtual ValueEntityBase & getValueIndex(Index i) = 0;
		virtual ~ColourValue() {}
	};

	class TransformValue
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

		virtual ValueEntityBase & getValueIndex(Aspect a, Index i) = 0;
		virtual ~TransformValue() {}
	};

	class DefaultValueListenerEntity : public ValueEntityBase
	{
		virtual void addListener(ValueEntityListener * listener) override { listeners.insert(listener); }
		virtual void removeListener(ValueEntityListener * listener) override { listeners.erase(listener); }

	protected:

		void notifyListeners()
		{
			for (auto l : listeners)
				l->valueEntityChanged(nullptr, this);
		}

	private:
		std::set<ValueEntityListener *> listeners;
	};

	template<typename Transformer = VirtualTransformer<ValueT>, typename Formatter = VirtualFormatter<ValueT>>
	class SelfcontainedValue : public DefaultValueListenerEntity
	{
	public:
		SelfcontainedValue(Transformer * transformer, Formatter * formatter)
			: internalValue(0)
			, transformer(transformer)
			, formatter(formatter)
		{

		}

		virtual VirtualTransformer<ValueT> & getTransformer() override { return *transformer; }
		virtual VirtualFormatter<ValueT> & getFormatter() override { return *formatter; }
		virtual ValueT getNormalizedValue() override { return internalValue; }
		virtual void setNormalizedValue(ValueT value) override { internalValue = value; notifyListeners(); }

	private:
		double internalValue;
		Transformer * transformer;
		Formatter * formatter;
	};

	template<typename Transformer = VirtualTransformer<ValueT>, typename Formatter = VirtualFormatter<ValueT>>
	class CompleteValue : public SelfcontainedValue<Transformer, Formatter>
	{
	public:

		CompleteValue()
			: SelfcontainedValue(&hostedTransformer, &hostedFormatter)
		{

		}

	private:
		Transformer hostedTransformer;
		Formatter hostedFormatter;
	};

	class CompleteColour 
		: private HexFormatter<ValueT>
		, public ColourValue
	{
	public:

		CompleteColour()
			: range(0, 0xFF)
			, values{
				{ &range, this },
				{ &range, this },
				{ &range, this },
				{ &range, this },
			}
		{

		}

		virtual ValueEntityBase & getValueIndex(Index i) override { return values[i]; }

		SelfcontainedValue<> values[4];

	private:
		LinearRange<ValueT> range;
	};

	template<typename UIParameterView>
	class ParameterValueWrapper : public DefaultValueListenerEntity, UIParameterView::Listener
	{
	public:

		ParameterValueWrapper(UIParameterView * parameterToRef = nullptr)
			: parameterView(nullptr)
		{
			setParameterReference(parameterToRef);
		}

		void setParameterReference(UIParameterView * parameterView) 
		{ 
			if (parameterView != nullptr)
			{
				parameterView->removeListener(this);
			}
			parameterView = parameterView; 
			if (parameterView != nullptr)
			{
				parameterView->addListener(this);
			}
		}

		void parameterChanged(Parameters::Handle, Parameters::Handle, UIParameterView * parameterThatChanged)
		{
			if (parameterView == parameterThatChanged)
				notifyListeners();

			CPL_RUNTIME_EXCEPTION("Unknown parameter callback; corruption");
		}

		virtual VirtualTransformer<ValueT> & getTransformer() override { return parameterView->getTransformer(); }
		virtual VirtualFormatter<ValueT> & getFormatter() override { return parameterView->getFormatter(); }
		virtual ValueT getNormalizedValue() override { return parameterView->getValueNormalized<ValueT>(); }
		virtual void setNormalizedValue(ValueT value) override { parameterView->updateFromUINormalized(static_cast<UIParameterView::ValueType>(value)); }
		virtual void beginChangeGesture() override { return parameterView->beginChangeGesture(); }
		virtual void endChangeGesture() override { return parameterView->endChangeGesture(); }
		virtual std::string getContextualName() override { return parameterView->getExportedName(); }

	private:
		UIParameterView * parameterView;
	};

	
	// TODO: specialize on std::is_base_of<FormattedParameter, UIParamerView::ParameterType>
	template<typename UIParameterView>
	class ParameterValue
		: public ParameterValueWrapper<UIParameterView>
	{
		typedef typename UIParameterView::ValueType ValueType;
		typedef typename UIParameterView::ParameterType ParameterType;
		typedef typename Parameters::BundleUpdate<UIParameterView>::Record Entry;

		friend class Updater;

	public:

		ParameterValue(std::string name, typename ParameterType::Transformer & tsf, typename ParameterType::Formatter & formatter)
			: parameter(std::move(name), tsf, &formatter)
		{

		}

		Parameters::SingleUpdate<UIParameterView> * generateUpdateRegistrator(bool isAutomatable = true, bool canChangeOthers = false)
		{
			// TODO: reference-count at least?
			return new Updater(this, isAutomatable, canChangeOthers);
		}

		ParameterType parameter;

	private:

		class Updater : public Parameters::SingleUpdate<UIParameterView>
		{
		public:
			Updater(ParameterValue<UIParameterView> * parent, bool isAutomatable = true, bool canChangeOthers = false)
			{
				entry.parameter = &parent->parameter;
				entry.shouldBeAutomatable = isAutomatable;
				entry.canChangeOthers = canChangeOthers;
				entry.uiParameterView = nullptr;

				parameterQuery = &entry;
			}

		private:
			virtual void parametersInstalled() override
			{
				parent->setParameterReference(entry.uiParameterView);
				delete this;
			}
			ParameterValue<UIParameterView> * parent;
			Entry entry;
		};

	};


	template<typename UIParameterView>
	class ParameterColourValue 
		: public ColourValue
		, public Parameters::BundleUpdate<UIParameterView>
	{

	public:
		typedef typename UIParameterView::ValueType ValueType;
		typedef typename UIParameterView::ParameterType ParameterType;
		typedef typename Parameters::BundleUpdate<UIParameterView>::Record Entry;

		class SharedBehaviour : private HexFormatter<ValueType>
		{
		public:
			SharedBehaviour() : range(0, 0xFF), context("CC") {}

			VirtualFormatter<ValueType> & getFormatter() { return *this; }
			VirtualTransformer<ValueType> & getTransformer() { return range; }
			const std::string & getContext() { return context; }
		private:
			LinearRange<ValueType> range;
			std::string context;
		};

		ParameterColourValue(SharedBehaviour & b)
			: r("R", b.getTransformer(), &b.getFormatter())
			, g("G", b.getTransformer(), &b.getFormatter())
			, b("B", b.getTransformer(), &b.getFormatter())
			, a("A", b.getTransformer(), &b.getFormatter())
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

		void generateInfo() override
		{
			parameters = std::make_unique<std::vector<Entry>>();
			auto add = [&](ParameterType & p) { parameters->push_back(Entry{ &p, true, false }); };
			add(r); add(g); add(b); add(a);
		}

		ValueEntityBase & getValueIndex(Index i) override
		{
			return values[i];
		}

		ParameterType r, g, b, a;
		std::array<ParameterValueWrapper<UIParameterView>, 4> values;

	private:

		std::unique_ptr<std::vector<Entry>> parameters;
		SharedBehaviour & behaviour;
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

		class SharedBehaviour : private BasicFormatter<ValueType>
		{
		public:
			SharedBehaviour()
				: context("TSF")
				, degreeFormatter("degs")
				, degreeRange(0, 360)
				, magnitudeRange(0, 50)
			{

			}

			VirtualFormatter<ValueType> & getDegreeFormatter() { return degreeFormatter; }
			VirtualFormatter<ValueType> & getDefaultFormatter() { return *this; }
			VirtualTransformer<ValueType> & getMagTransformer() { return magnitudeRange; }
			VirtualTransformer<ValueType> & getDegreeTransformer() { return degreeRange; }
			const std::string & getContext() { return context; }

		private:
			std::string context;
			UnitFormatter<ValueType> degreeFormatter;
			LinearRange<ValueType> magnitudeRange, degreeRange;
		};

		ParameterTransformValue(SharedBehaviour & b)
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
		SharedBehaviour & behaviour;
	};
};

#endif