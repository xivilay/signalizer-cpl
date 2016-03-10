/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2016 Janus Lynggaard Thorborg (www.jthorborg.com)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	See \licenses\ for additional details on licenses associated with this program.

**************************************************************************************
 
	 file:GraphicComponents.h
		
		Wrappers and graphic classes that can be used for controls, images, etc.
	
 *************************************************************************************/

#ifndef CPL_GRAPHICCOMPONENTS_H
	#define CPL_GRAPHICCOMPONENTS_H

	#include "Common.h"
	#include "Utility.h"
	#include <map>
	#include <cstdint>
	#include "CBaseControl.h"
	#include <string>
	#include "GUIUtils.h"
	#include "CCtrlEditSpace.h"
	#include "gui/Controls.h"
	#include "CMutex.h"
	#include "gui/DesignBase.h"

	#include "CSerializer.h"

	namespace cpl
	{


		/*********************************************************************************************

			RAII wrapper around images, loaded at runtime

		*********************************************************************************************/
		class CImage : public cpl::CMutex::Lockable
		{
			std::string path;
			juce::Image internalImage;
			juce::ScopedPointer<Drawable> drawableImage;

		public:
			CImage(const std::string & inPath);
			CImage();
			void setPath(const std::string & inPath);
			bool load();
			juce::Image & getImage();
			juce::Drawable * getDrawable();
			virtual ~CImage();
		};


		struct DrawableWithLock
		{

			DrawableWithLock(juce::Drawable * c, cpl::CMutex::Lockable * l)
				: content(c), lock(l)
			{

			}

			bool valid()
			{
				return content && lock;
			}

			void null()
			{
				lock = nullptr;
				content = nullptr;
			}

			juce::Drawable * content;
			cpl::CMutex::Lockable * lock;

		};


		/*********************************************************************************************

			Manages all resources used by this program, statically

		*********************************************************************************************/
		class CResourceManager
		:
			public juce::DeletedAtShutdown,
			public Utility::CNoncopyable
		{


		public:
			bool loadResources();
			// singleton instance
			DrawableWithLock getResource(const std::string & name);
			juce::ScopedPointer<juce::Drawable> getCopyOfDrawable(const std::string & name);
			//CVectorIcon getVectorResource(const std::string & name);
			const juce::Image & getImage(const std::string & name);
			static CResourceManager & instance();

		private:
			static CResourceManager * internalInstance;
			std::map<std::string, CImage> resources;
			bool isResourcesLoaded;
			~CResourceManager();
			CResourceManager();
		};

		class CVectorResource
		{
		public:
			CVectorResource()
				: svg(nullptr, nullptr)
			{
			}

			CVectorResource(std::string name)
				: svg(nullptr, nullptr)
			{
				associate(name);
			}

			template<typename T>
				void renderImage(juce::Rectangle<T> size, juce::Colour colour = juce::Colour(0, 0, 0), float opacity = 1.f)
				{
					if (!svg.valid())
						return;
					// images has to be at least 1 pixel.
					if (size.getWidth() < 1 || size.getHeight() < 1)
						return;

					CMutex lock(svg.lock);

					auto intSize = size.template toType<int>();

					if (intSize != image.getBounds())
						image = juce::Image(image.ARGB, intSize.getWidth(), intSize.getHeight(), true);

					juce::Graphics g(image);
					svg.content->replaceColour(Colours::black, colour);
					svg.content->drawWithin(g, size.withPosition(0, 0).toFloat(), juce::RectanglePlacement::centred, opacity);
				}

			static juce::Image renderSVGToImage(const std::string & path, juce::Rectangle<int> size, juce::Colour colour = juce::Colour(0, 0, 0), float opacity = 1.f)
			{
				auto resource = CResourceManager::instance().getResource(path);

				if (resource.valid())
				{
					juce::Image image(juce::Image::ARGB, size.getWidth(), size.getHeight(), true);
					juce::Graphics g(image);
					CMutex lock(resource.lock);
					resource.content->replaceColour(Colours::black, colour);
					resource.content->drawWithin(g, size.withPosition(0, 0).toFloat(), juce::RectanglePlacement::centred, opacity);
					return image;
				}
				return juce::Image::null;
			}

			juce::Image & getImage() { return image; }

			bool associate(std::string path)
			{
				auto content = CResourceManager::instance().getResource(path);
				
				if (content.valid())
				{
					svg = content;
					return true;
				}
				
				svg.null();
				return false;
			}

		private:

			DrawableWithLock svg;
			juce::Image image;
		};







		class CEditSpaceSpawner 
		: 
			public juce::MouseListener,
			public Utility::CNoncopyable,
			public Utility::DestructionServer<CCtrlEditSpace>::Client,
			public juce::ComponentListener
		{
		public:
			// http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
			CEditSpaceSpawner(juce::Component & parentToControl);

		protected:

			void appearWith(juce::Component & component);
			void disappear();
			virtual void onObjectDestruction(const CCtrlEditSpace::ObjectProxy & dyingSpace) override;
			virtual void componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) override;
			virtual void mouseDoubleClick(const juce::MouseEvent & e) override;
			
			virtual void mouseDown(const juce::MouseEvent & e) override;

			bool isEditSpacesOn;
			bool recursionEdit;
			juce::Component & parent;
			std::unique_ptr<cpl::CCtrlEditSpace> currentEditSpace;

		private:
			CEditSpaceSpawner(const CEditSpaceSpawner &) = delete;
			
			class OpaqueComponent
				: public juce::Component
			{
				void paint(juce::Graphics & g) override
				{
					g.fillAll(GetColour(ColourEntry::deactivated));
					
				}
				
				
			};
			
			OpaqueComponent dialog;
		};



		/*********************************************************************************************

			Button interface

		*********************************************************************************************/
		class CButton2 : public juce::DrawableButton, public CBaseControl
		{
			std::string texts[2];
			bool multiToggle;
		public:
			CButton2();
			CButton2(const std::string & text, const std::string & textToggled = "", CCtrlListener * list = nullptr);
			void paintOverChildren(juce::Graphics & g) override;
			void setMultiToggle(bool toggle);
			void bSetInternal(iCtrlPrec_t newValue) override;
			void bSetValue(iCtrlPrec_t newValue, bool sync = false) override;
			iCtrlPrec_t bGetValue() const override;
		};
		
		
		class CRenderButton : public juce::Button, public CBaseControl
		{
			juce::Colour c;
			juce::String texts[2];
			bool toggle;
		public:
			CRenderButton(const std::string & text, const std::string & textToggled = "");
			~CRenderButton();
			
			void setToggleable(bool isAble);
			void bSetInternal(iCtrlPrec_t newValue) override;
			void bSetValue(iCtrlPrec_t newValue, bool sync = false) override;
			iCtrlPrec_t bGetValue() const override;
			void setUntoggledText(const std::string &);
			void setToggledText(const std::string &);
			void setButtonColour(juce::Colour newColour);
			std::string bGetTitle() const override;
			void bSetTitle(const std::string & ) override;
			juce::Colour getButtonColour();
		private:
			void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown);

		};

		/*********************************************************************************************

			Toggle interface

		*********************************************************************************************/
		class CToggle : public juce::ToggleButton, public CBaseControl
		{
			const juce::Image & cbox;
			juce::String text;
		public:
			CToggle();
			void paint(juce::Graphics & g) override;
			iCtrlPrec_t bGetValue() const override;
			void bSetInternal(iCtrlPrec_t newValue) override;
			void bSetValue(iCtrlPrec_t newValue, bool sync = false) override;
			void bSetText(const std::string & in) override;
		};
		/*********************************************************************************************

			Textlabel interface

		*********************************************************************************************/
		class CTextLabel : public juce::Component
		{
		protected:
			juce::String text;
			float size;
			CColour colour;
			juce::Justification just;
		public:
			CTextLabel();
			void setFontSize(float newSize);
			void setColour(CColour newColour);
			virtual void setText(const std::string & newText);
			virtual void paint(juce::Graphics & g) override;
			void setPos(int x, int y);
			void setJustification(juce::Justification j) { just = j; }
		};
		/*********************************************************************************************

			Greenlinetester. Derive from this if you are uncertain that you are getting painted -
			Will draw a green line.

		*********************************************************************************************/
		class CGreenLineTester : public juce::Component
		{
			void paint(juce::Graphics & g)
			{
				g.setColour(juce::Colours::green);
				g.drawLine(juce::Line<float>(0.f, 0.f, static_cast<float>(getWidth()), static_cast<float>(getHeight())),1);
				g.setColour(juce::Colours::blue);
				g.drawRect(getBounds().withZeroOrigin().toFloat(), 0.5f);
			}
		};
		/*********************************************************************************************

			Name says it all. Holds a virtual container of larger size, that is scrollable.

		*********************************************************************************************/
		class CScrollableContainer : public juce::Component, public CBaseControl
		{
		protected:
			juce::ScrollBar * scb;
			Component * virtualContainer;
			const juce::Image * background;
		public:
			CScrollableContainer();
			void bSetSize(const CRect & in);
			int getVirtualHeight();
			void setVirtualHeight(int height);
			void bSetValue(iCtrlPrec_t newVal);
			iCtrlPrec_t bGetValue();
			void setBackground(const juce::Image * b) { background = b;}
			void setBackground(const juce::Image & b) {	background = &b; }
			juce::ScrollBar * getSCB() { return scb; }
			juce::Component * getVContainer() { return virtualContainer; }
			void scrollBarMoved(juce::ScrollBar * b, double newRange);
			virtual void paint(juce::Graphics & g) override;
			virtual ~CScrollableContainer() CPL_llvm_DummyNoExcept;

		};
		/*********************************************************************************************

			Same as CTextLabel, however is protected using a mutex

		*********************************************************************************************/
		class CTextControl : public CTextLabel, public CBaseControl
		{
		public:
			CTextControl();
			void bSetText(const std::string & newText) override;
			std::string bGetText() const override;
			void paint(juce::Graphics & g) override;

		};


	};
#endif