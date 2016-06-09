#ifndef CPL_PARAMETERSYSTEM_H
#define CPL_PARAMETERSYSTEM_H

#include <atomic>
#include <string>
#include "../../ConcurrentServices.h"
#include <vector>
#include "../../gui/Tools.h"
#include "CustomFormatters.h"
#include "CustomTransforms.h"


namespace cpl
{
	template<typename T>
	class ZeroOneClamper
	{
	public:
		T operator()(T arg) { return std::max((T)0, std::min((T)1, arg)); }
	};


	// Concecpt of Parameters: has T getValue(), setValue(T), std::string getName()

	template<typename T, typename class TransformerType = VirtualTransformer<T>, typename class Restricter = ZeroOneClamper<T>>
	class /* alignas(CPL_CACHEALIGNMENT) */ ThreadedParameter : Utility::CNoncopyable
	{
	public:

		typedef TransformerType Transformer;

		ThreadedParameter(const std::string & name, Transformer & parameterTransformer)
			: transformer(parameterTransformer), name(name), value(0)
		{

		}

		T getValue() { return value.load(std::memory_order_acquire);}
		void setValue(T newValue) { value.store(Restricter()(newValue), std::memory_order_release); }

		std::string getName() { return name; }
		Transformer & getTransformer() { return transformer; }

	private:

		Transformer & transformer;
		std::string name;
		std::atomic<T> value;
	};

	template<typename T, class BaseParameter, typename class FormatterType = VirtualFormatter<T>>
	class FormattedParameter 
		: public BaseParameter
		, private BasicFormatter<T> // (compression/EBO)
	{
	public:
		typedef FormatterType Formatter;

		FormattedParameter(const std::string & name, typename BaseParameter::Transformer & transformer, Formatter * formatterToUse = nullptr)
			: BaseParameter(name, transformer), formatter(formatterToUse)
		{

		}

		Formatter & getFormatter() { return formatter ? *formatter : *this; }
		bool setFormatter(Formatter & formatterToUse) { return formatter = &formatterToUse; }

	private:
		Formatter * formatter;
	};


	namespace Parameters
	{
		typedef int Handle;

		enum UpdateFlags
		{
			/// <summary>
			/// Nothing recieves a notification
			/// </summary>
			None = 1 << 0,
			/// <summary>
			/// Whatever realtime system is there, recieves a notification.
			/// For instance, the audio thread (and the host)
			/// </summary>
			RealTimeSubSystem = 1 << 1,
			/// <summary>
			/// UI receives a notification
			/// </summary>
			UI = 1 << 2,
			/// <summary>
			/// Everything recieves a notification
			/// </summary>
			All = UI | RealTimeSubSystem
		};

		class UserContent
		{
		public:
			virtual ~UserContent() {};
		};

		struct UserContentDeleter
		{
			void operator()(UserContent * content) { if (doDelete) delete content; }
			bool doDelete = true;
		};

		template<typename UIParameterView>
		struct CallbackParameterRecord
		{
			typename UIParameterView::ParameterType * parameter;
			bool shouldBeAutomatable;
			bool canChangeOthers;
			Parameters::Handle handle;
			UIParameterView * uiParameterView;
		};

		class CallbackRecordInterface
		{
		public:
			/// <summary>
			/// A callback when all parameters (including any nested ones) has been installed.
			/// At this point, Entry[n].uiParameterView is a stable reference for the life time of the parameter system
			/// </summary>
			virtual void parametersInstalled() = 0;
			/// <summary>
			/// Called just before any parameter(s) are queried
			/// </summary>
			virtual void generateInfo() {};
			virtual ~CallbackRecordInterface() {}
		};

		template<typename UIParameterView>
		class BundleUpdate : public CallbackRecordInterface
		{
		public:
			typedef Parameters::CallbackParameterRecord<UIParameterView> Record;
			typedef BundleUpdate<UIParameterView> QualifedBundle;

			/// <summary>
			/// Returns a short, semantic name for this group of parameters (for instance, a widget name
			/// containing them)
			/// </summary>
			virtual const std::string & getBundleContext() const noexcept = 0;
			/// <summary>
			/// Queries a list of parameters, and fills in a reference to a UIParameterView.
			/// Returned data must be valid until parametersInstalled() has been called.
			/// </summary>
			virtual std::vector<Record> & queryParameters() = 0;

			/// <summary>
			/// If this bundle contains other owned bundles, it can override this function to return
			/// non-null
			/// </summary>
			virtual std::vector<QualifedBundle *> * getNestedChilds() { return nullptr; }

		};

		template<typename UIParameterView>
		class SingleUpdate : public CallbackRecordInterface
		{
		public:
			/// <summary>
			/// Initialize to a valid object after generateInfo() has happened. Not needed after parametersInstalled().
			/// </summary>
			Parameters::CallbackParameterRecord<UIParameterView> * parameterQuery;
		};

	};






	template<class T, typename InternalFrameworkType, typename BaseParameter>
	class ParameterGroup 
		: public CSerializer::Serializable
		, private DestructionNotifier::EventListener
	{
	public:

		typedef typename BaseParameter::Transformer Transformer;
		typedef typename BaseParameter::Formatter Formatter;
		typedef InternalFrameworkType FrameworkType;
		
		typedef ParameterGroup<T, InternalFrameworkType, BaseParameter> QualifiedGroup;

		static const Parameters::Handle InvalidHandle = -1;

		class AutomatedProcessor
		{
		public:
			/// <summary>
			/// The sematics of this function is to transmit the change message,
			/// but not proceed with any setParameter() calls
			/// </summary>
			virtual void automatedTransmitChangeMessage(int parameter, FrameworkType value) = 0;
			virtual void automatedBeginChangeGesture(int parameter) = 0;
			virtual void automatedEndChangeGesture(int parameter) = 0;
			virtual ~AutomatedProcessor() {}
		};

		class UIParameterView;

		class UIListener
		{
		public:
			virtual void parameterChanged(Parameters::Handle globalHandle, Parameters::Handle localHandle, UIParameterView * parameter) = 0;
		};

		class UIParameterView
		{
		public:
			friend class ParameterGroup;
			typedef UIListener Listener;
			typedef T ValueType;
			typedef BaseParameter ParameterType;
			UIParameterView(
				QualifiedGroup * parentToRef, 
				BaseParameter * parameterToRef, 
				Parameters::Handle handleOfThis, 
				bool paramIsAutomatable = true, 
				bool paramCanChangeOthers = false, 
				std::string nameContext = ""
			)
				: parent(parentToRef)
				, handle(handleOfThis)
				, parameter(parameterToRef)
				, isAutomatable(paramIsAutomatable)
				, canChangeOthers(paramCanChangeOthers)
				, nameContext(std::move(nameContext))
			{

			}

			UIParameterView(UIParameterView && other)
				: parent(other.parent)
				, handle(other.handle)
				, parameter(other.parameter)
				, isAutomatable(other.isAutomatable)
				, canChangeOthers(other.canChangeOthers)
				, nameContext(std::move(other.nameContext))
			{

			}

			ParameterType * getParameter() noexcept { return parameter; }

			std::string getExportedName() { return parent->prefix + nameContext + parameter->getName(); }
			std::string getLocalName() { return parameter->getName(); }
			Parameters::Handle getHandle() { return handle; }
			void addListener(UIListener * listener) { parent->addUIListener(handle, listener); }
			void removeListener(UIListener * listener) { parent->removeUIListener(handle, listener); }

			void updateFromUINormalized(ValueType value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All) 
			{ 
				parent->updateFromUINormalized(handle, value, flags); 
			}

			void updateFromProcessorNormalized(ValueType value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All) 
			{ 
				parent->updateFromProcessorNormalized(handle, value, flags); 
			}

			void updateFromHostNormalized(ValueType value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All) 
			{ 
				parent->updateFromHostNormalized(handle, value, flags); 
			}

			void beginChangeGesture() { parent->beginChangeGesture(handle); }
			void endChangeGesture() { parent->beginChangeGesture(handle); }

			void updateFromUITransformed(ValueType value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All) 
			{ 
				return updateFromUINormalized(parameter->getTransformer().normalize(value), flags);
			}

			void updateFromProcessorTransformed(ValueType value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All)
			{ 
				return updateFromProcessorNormalized(parameter->getTransformer().normalize(value), flags);
			}

			bool updateFromUIStringTransformed(const std::string & value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All)
			{
				T interpretedValue;
				if(parameter->getFormatter()->interpret(value, interpretedValue))
				{
					return updateFromUINormalized(parameter->getTransformer().normalize(interpretedValue), flags);
				}
				return false;
			}

			template<typename Ret = T>
				Ret getValueNormalized() { return static_cast<Ret>(parameter->getValue()); }
			template<typename Ret = T>
				Ret getValueTransformed() { return static_cast<Ret>(parameter->getTransformer().normalize(parameter->getValue())); }


			std::string getDisplayText()
			{
				std::string buf;
				parameter->getFormatter().format(parameter->getTransformer().transform(parameter->getValue()), buf);
				return std::move(buf);
			}

			Formatter & getFormatter() { return parameter->getFormatter(); }
			Transformer & getTransformer() { return parameter->getTransformer(); }

		private:

			std::string nameContext;
			Parameters::Handle handle;
			BaseParameter * parameter;
			bool isAutomatable;
			bool canChangeOthers;
			ABoolFlag changedFromProcessor;
			std::set<UIListener *> uiListeners;
			QualifiedGroup * parent;
		};



		ParameterGroup(const std::string & name, const std::string & exportPrefix, AutomatedProcessor & processorToAutomate, int parameterOffset = 0)
			: processor(processorToAutomate), offset(parameterOffset), groupName(name), prefix(exportPrefix), isSealed(false)
		{
			bundleInstalledReferences = std::make_unique<std::vector<BundleInstallReference>>();
			singleInstalledReferences = std::make_unique<std::vector<SingleInstallReference>>();
		}

		void serialize(CSerializer::Archiver & archive, Version v) override
		{
			if (!isSealed)
				CPL_RUNTIME_EXCEPTION("Parameter system being serialized without being sealed");

			archive << groupName;
			archive << offset;

			for (std::size_t i = 0; i < containedParameters.size(); ++i)
			{
				archive << containedParameters[i].getValueNormalized();
			}
		}

		void deserialize(CSerializer::Builder & builder, Version v) override
		{
			if (!isSealed)
				CPL_RUNTIME_EXCEPTION("Parameter system being deserialized without being sealed");

			builder >> groupName;
			builder >> offset;

			for (std::size_t i = 0; i < containedParameters.size(); ++i)
			{
				T value;
				builder >> value;
				containedParameters[i].updateFromUINormalized(value);
			}
		}

		/// <summary>
		/// Sets some user-generated content to be associated with this parameter group.
		/// The content ownership can be transferred to the parameter group (default).
		/// You can for instance store all your parameters, formatters and transformers in a 
		/// class deriving from the Parameters::UserContent.
		/// </summary>
		void setUserData(Parameters::UserContent * content, bool releaseOnDestruction = true)
		{
			userContent.reset(content);
			userContent.get_deleter().doDelete = !!releaseOnDestruction;
		}

		/// <summary>
		/// Returns anything previously set with setUserData() - as of such, may be null.
		/// </summary>
		Parameters::UserContent * getUserContent() noexcept
		{
			return userContent.get();
		}

		/// <summary>
		/// This function must only be called during initialization, ie. before any audio callbacks are done.
		/// Additionally, it only makes sense to call it on the UI thread.
		/// </summary>
		Parameters::Handle registerParameter(BaseParameter * param, bool shouldBeAutomatable = true, bool canChangeOthers = false, std::string nameContext = "")
		{
			if (isSealed)
				CPL_RUNTIME_EXCEPTION("Parameters registered to the system while it's sealed");
			auto pos = containedParameters.size();
			containedParameters.emplace_back(this, param, (Parameters::Handle)pos, shouldBeAutomatable, canChangeOthers, nameContext);
			return static_cast<Parameters::Handle>(pos + offset);
		}
		
		
		void registerParameterBundle(Parameters::BundleUpdate<UIParameterView> * bundle, std::string contextStack = "")
		{
			contextStack += bundle->getBundleContext();
			bundle->generateInfo();
			auto & parameters = bundle->queryParameters();
			for (auto & parameter : parameters)
			{
				parameter.handle = registerParameter(parameter.parameter, parameter.shouldBeAutomatable, parameter.canChangeOthers, contextStack);
			}

			bundleInstalledReferences.get()->push_back({ bundle, &parameters });

			if (auto bundleChilds = bundle->getNestedChilds())
			{
				for (auto & childBundle : *bundleChilds)
				{
					registerParameterBundle(childBundle, contextStack);
				}
			}

		}

		void registerSingleParameter(Parameters::SingleUpdate<UIParameterView> * singleRef)
		{
			singleRef->generateInfo();
			singleRef->parameterQuery->handle = registerParameter(
				singleRef->parameterQuery->parameter, 
				singleRef->parameterQuery->shouldBeAutomatable, 
				singleRef->parameterQuery->canChangeOthers
			);
			singleInstalledReferences.get()->push_back({ singleRef, singleRef->parameterQuery });
		}

		void seal()
		{
			isSealed = true;
			for (auto & ref : *bundleInstalledReferences.get())
			{
				for (auto & parameter : *ref.records)
				{
					parameter.uiParameterView = findParameter(parameter.handle);
				}

				ref.parent->parametersInstalled();
			}

			for (auto & ref : *singleInstalledReferences.get())
			{
				ref.record->uiParameterView = findParameter(ref.record->handle);
				ref.parent->parametersInstalled();
			}

			bundleInstalledReferences = nullptr;
			singleInstalledReferences = nullptr;

		}

		/// <summary>
		/// Only safe to call on the UI thread.
		/// </summary>
		void addUIListener(Parameters::Handle handle, UIListener * listener)
		{
			containedParameters.at(handle - offset).uiListeners.insert(listener);
		}

		/// <summary>
		/// Only safe to call on the UI thread.
		/// </summary>
		void removeUIListener(Parameters::Handle handle, UIListener * listener)
		{
			containedParameters.at(handle - offset).uiListeners.insert(listener);
		}

		/// <summary>
		/// Safe to call from any thread
		/// </summary>
		void updateFromProcessorNormalized(Parameters::Handle handle, T value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All)
		{
			UIParameterView & p = containedParameters.at(handle - offset);

			p.parameter->setValue(value);
			if(flags & Parameters::UpdateFlags::UI)
				p.changedFromProcessor = true;

			if (flags & Parameters::UpdateFlags::RealTimeSubSystem && p.isAutomatable)
			{
				processor.automatedTransmitChangeMessage(handle - offset, static_cast<FrameworkType>(value));
			}
		}

		/// <summary>
		/// Should only be called from a host callback (setParameter)
		/// </summary>
		void updateFromHostNormalized(Parameters::Handle handle, T value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All)
		{
			UIParameterView & p = containedParameters.at(handle - offset);

			p.parameter->setValue(value);
			if(flags & Parameters::UpdateFlags::UI)
				p.changedFromProcessor = true;

		}

		/// <summary>
		/// Only safe to call on the UI thread.
		/// </summary>
		void updateFromUINormalized(Parameters::Handle handle, T value, Parameters::UpdateFlags flags = Parameters::UpdateFlags::All)
		{
			UIParameterView & p = containedParameters.at(handle - offset);
			p.parameter->setValue(value);

			if (flags & Parameters::UpdateFlags::RealTimeSubSystem && p.isAutomatable)
			{
				processor.automatedTransmitChangeMessage(handle - offset, static_cast<FrameworkType>(value));
			}

			if (flags & Parameters::UpdateFlags::UI)
			{
				for (auto listener : p.uiListeners)
				{
					listener->parameterChanged(handle, handle - offset, &p);
				}
			}

		}

		/// <summary>
		/// Should be called regularly to recieve notifications from the processor
		/// thread
		/// </summary>
		void pulseUI()
		{
			for (std::size_t i = 0; i < containedParameters.size(); ++i)
			{
				if (containedParameters[i].changedFromProcessor.cas())
				{
					for (auto listener : containedParameters[i].Parameters::UIListeners)
					{
						listener->parameterChanged(static_cast<Parameters::Handle>(i) + offset);
					}
				}
			}
		}

		void beginChangeGesture(Parameters::Handle handle)
		{
			processor.automatedBeginChangeGesture(handle - offset);
		}

		void endChangeGesture(Parameters::Handle handle)
		{
			processor.automatedEndChangeGesture(handle - offset);
		}

		/// <summary>
		/// O(N)
		/// </summary>
		Parameters::Handle handleFromName(const std::string & name) const noexcept
		{
			for (std::size_t i = 0; i < containedParameters.size(); ++i)
			{
				if (name == containedParameters[i].parameter->getName())
				{
					return static_cast<Parameters::Handle>(i) + offset;
				}
			}
			
			return InvalidHandle;
		}

		/// <summary>
		/// Must only be called from the UI thread
		/// </summary>
		Parameters::Handle mapName(const std::string & name) const noexcept
		{
			auto it = nameMap.find(name);
			if (it == nameMap.end())
			{
				auto handle = findFromName(name);
				if (handle != InvalidHandle)
				{
					return (mapName[name] = handle) + offset;
				}
			}
			else
			{
				return it->second + offset;
			}
			return InvalidHandle;
		}

		std::size_t size() const noexcept
		{
			return containedParameters.size();
		}

		UIParameterView * findParameter(Parameters::Handle handle)
		{
			if (!isSealed)
				CPL_RUNTIME_EXCEPTION("ParameterView being acquired while the system isn't sealed");

			handle -= offset;
			if (handle != InvalidHandle && static_cast<std::size_t>(handle) < containedParameters.size())
			{
				return &containedParameters[handle];
			}
			return nullptr;
		}

		UIParameterView * findParameter(const std::string & name) noexcept
		{
			if (!isSealed)
				CPL_RUNTIME_EXCEPTION("ParameterView being acquired while the system isn't sealed");
			return findParameter(mapName(name));
		}

		const std::string & getName() const noexcept { return groupName; }


	private:

		void onServerDestruction(DestructionNotifier * notif)
		{
			for (std::size_t i = 0; i < containedParameters.size(); ++i)
			{
				//containedParameters[i].uiListeners.erase(static_cast<UIListener *>(notif));
			}
		}

		bool isSealed;

		struct BundleInstallReference
		{
			Parameters::BundleUpdate<UIParameterView> * parent;
			std::vector<typename Parameters::BundleUpdate<UIParameterView>::Record> * records;
		};

		struct SingleInstallReference
		{
			Parameters::SingleUpdate<UIParameterView> * parent;
			Parameters::CallbackParameterRecord<UIParameterView> * record;
		};

		std::unique_ptr<std::vector<BundleInstallReference>> bundleInstalledReferences;
		std::unique_ptr<std::vector<SingleInstallReference>> singleInstalledReferences;

		std::unique_ptr<Parameters::UserContent, Parameters::UserContentDeleter> userContent;
		std::string prefix;
		std::string groupName;
		std::map<std::string, Parameters::Handle> nameMap;
		Parameters::Handle offset;
		AutomatedProcessor & processor;
		std::vector<UIParameterView> containedParameters;
	};


	class ValueView
	{
		class Listener
		{

		};
	};

};

#endif