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

	class ContextualName
	{
	public:
		virtual std::string getContextualName() { return{}; }
		virtual ~ContextualName() {}
	};

	class ValueEntityBase 
		: public CSerializer::Serializable
		, public ContextualName
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

	class ValueGroup : public ContextualName
	{
	public:
		virtual ValueEntityBase & getValueIndex(std::size_t i) = 0;
		virtual std::size_t getNumValues() const noexcept = 0;
	};

	class ColourValue : public ValueGroup
	{
	public:
		enum Index
		{
			R, G, B, A, ColourChannels
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
		template<std::size_t bits, typename ret = std::uint32_t>
		typename std::enable_if<bits < 32, ret>::type getIntValueFor(Index i)
		{
			auto cap = (1 << bits);
			auto fpoint = getValueIndex(i).getNormalizedValue();
			return fpoint == (ValueT)1.0 ? static_cast<ret>(cap - 1) : static_cast<ret>(fpoint * cap);
		}

		virtual std::size_t getNumValues() const noexcept override { return ColourChannels; }

		virtual std::string getContextualName() { return{}; }
	};

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

		virtual ValueEntityBase & getValueIndex(Aspect a, Index i) = 0;
		virtual ValueEntityBase & getValueIndex(std::size_t i) override
		{ 
			std::div_t res = std::div(i, 3); 
			return getValueIndex((Aspect)res.quot, (Index)res.rem);
		}

		virtual std::size_t getNumValues() const noexcept override { return 9; }
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

		virtual ValueEntityBase & getValueIndex(std::size_t i) override { return values[i]; }

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

		void setParameterReference(UIParameterView * parameterReference) 
		{ 
			if (parameterView != nullptr)
			{
				parameterView->removeListener(this);
			}
			parameterView = parameterReference; 
			if (parameterView != nullptr)
			{
				parameterView->addListener(this);
			}
		}

		void parameterChanged(Parameters::Handle, Parameters::Handle, UIParameterView * parameterThatChanged)
		{
			if (parameterView != parameterThatChanged)
			{
				CPL_RUNTIME_EXCEPTION("Unknown parameter callback; corruption");
			}

			notifyListeners();
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
			Updater(ParameterValue<UIParameterView> * parentToRef, bool isAutomatable = true, bool canChangeOthers = false)
				: parent(parentToRef)
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

		ParameterColourValue(SharedBehaviour & b, const std::string & name = "")
			: r("R", b.getTransformer(), &b.getFormatter())
			, g("G", b.getTransformer(), &b.getFormatter())
			, b("B", b.getTransformer(), &b.getFormatter())
			, a("A", b.getTransformer(), &b.getFormatter())
			, behaviour(b)
			, contextName(name)
		{

		}

		virtual std::string getContextualName() override { return contextName; }

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

		ValueEntityBase & getValueIndex(std::size_t i) override
		{
			return values[i];
		}

		ParameterType r, g, b, a;
		std::array<ParameterValueWrapper<UIParameterView>, 4> values;

	private:
		std::string contextName;
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
				: context("tsf")
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