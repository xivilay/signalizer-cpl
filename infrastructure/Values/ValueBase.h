#ifndef CPL_VALUEBASE_H
#define CPL_VALUEBASE_H

#include "../parameters/CustomFormatters.h"
#include "../parameters/CustomTransforms.h"
#include "../parameters/ParameterSystem.h"
#include "../../gui/Tools.h"
#include "../../Misc.h"
#include "../../CSerializer.h"
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

};

#endif