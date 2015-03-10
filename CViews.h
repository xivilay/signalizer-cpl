/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
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

	file:CViews.h
		
		Provides some base classes for views, that automatically supports
		openGL rendering, notification of death, serialization etc.

*************************************************************************************/

#ifndef _CVIEWS_H
	#define _CVIEWS_H

	#include <set>
	#include "Common.h"
	#include "CSerializer.h"
	#include "CToolTip.h"
	#include "GraphicComponents.h"

	namespace cpl
	{
		/*
			A CView is the base class of all views.
		*/
		class CView
		:
			public CSerializer::Serializable
		{
			
		public:
			class EventListener
			{
			public:
				virtual void onViewConstruction(CView * view) = 0;
				virtual void onViewDestruction(CView * view) = 0;
				
				virtual ~EventListener() {};
				
			};
			
			CView()
				: isFullScreen(false), isSynced(false), oglc(nullptr), bufferSwapInterval(0)
			{
			}
			

			virtual ~CView()
			{
				if (eventListeners.size())
				{
					// you must call notifyListeners() in your destructor!
					jassertfalse;
				}
			}

			virtual juce::Component * getWindow() = 0;
			virtual bool setFullScreenMode(bool toggle) { isFullScreen = toggle; return false; }
			bool getIsFullScreen() const { return isFullScreen; }
			virtual void addEventListener(EventListener * el) { eventListeners.insert(el); }
			virtual void removeEventListenerr(EventListener * el) { eventListeners.erase(el); }
			virtual void repaintMainContent() {};
			virtual void visualize() {};
			virtual void suspend() {};
			virtual void resume() {  };
			virtual void freeze() {};
			virtual void unfreeze() {};
			virtual void attachToOpenGL(juce::OpenGLContext & ctx) { detachFromOpenGL();  oglc = &ctx; }
			virtual void detachFromOpenGL(juce::OpenGLContext & ctx) 
			{ 
				if (oglc)
					jassert(&ctx == oglc);
				ctx.detach(); 
				oglc = nullptr; 
			}
			void detachFromOpenGL() { if (oglc) oglc->detach(); oglc = nullptr; }
			virtual std::unique_ptr<GraphicComponent> createEditor() { return nullptr; }

			bool isOpenGL() { return oglc != nullptr; }
			juce::OpenGLContext * getAttachedContext() const noexcept { return oglc; }
			bool shouldSynchronize() { return isSynced; }
			void setSyncing(bool shouldSync) { isSynced = shouldSync; }
			void setApproximateRefreshRate(int ms) { refreshRate = ms; }
			void setSwapInterval(int interval) { bufferSwapInterval = interval; }
		protected:

			void notifyDestruction()
			{
				for (auto listener : eventListeners)
					listener->onViewDestruction(this);
				eventListeners.clear();
			}

			bool isFullScreen;
			bool isSynced;
			// the rate at which the 2D ui gets refreshed (called from repaintMainContent)
			int refreshRate;
			// -1 == no swap buffer interval defined, all opengl rendering
			// should be triggered through repaintMainContent()
			// 0 == no cap on framerate, 1 = vsync, 2 and over are reciprocals of
			// current monitor refresh rate.
			int bufferSwapInterval;
			std::set<EventListener *> eventListeners;
			juce::OpenGLContext * oglc;
			
		};

		/*
			A subview is a stand-alone view, instantiable as-is.
		*/
		class CSubView
		: 
			public CView,
			public juce::Component
		{
			juce::Component * getWindow() override { return this; }

		};

		/*
			An openGL view is able to draw openGL through the renderOpenGL()
			function and standard 2D graphics through paint().
			Note that openGL may not be enabled, and you probably should
			enable software rendering as well - in paint(). You can test
			for openGL availability through isOpenGL().
		*/
		class COpenGLView
		:
			public CSubView,
			juce::OpenGLRenderer
		{
			void repaintMainContent() override
			{
				repaint();
				if (bufferSwapInterval < 0)
					oglc->triggerRepaint();
			}

			virtual void initOpenGL() {}
			virtual void closeOpenGL() {}

			void attachToOpenGL(juce::OpenGLContext & ctx) override
			{
				ctx.setRenderer(this);
				CView::attachToOpenGL(ctx);
				ctx.attachTo(*this);
			}
			void detachFromOpenGL(juce::OpenGLContext & ctx) override
			{
				CView::detachFromOpenGL(ctx);
				ctx.setRenderer(nullptr);
			}

		protected:

			void newOpenGLContextCreated() override
			{
				if (bufferSwapInterval >= 0)
					oglc->setSwapInterval(bufferSwapInterval);

				initOpenGL();
			}
			void openGLContextClosing() override
			{
				closeOpenGL();
			}

			virtual ~COpenGLView()
			{
				CView::detachFromOpenGL();
			}
		};

		/*
			A top-view is a view, that also handles tooltips and edit spaces
			for it's children. Has an associative style.
		*/
		class CTopView 
		: 
			public CView
		{

		public:
			CTopView(juce::Component * parent)
				: isTooltipsOn(false), tipWindow(nullptr), editSpawner(*parent)
			{
				parent->setLookAndFeel(&CLookAndFeel_CPL::defaultLook());
			}

		protected:

		private:
			bool isTooltipsOn;
			cpl::CEditSpaceSpawner editSpawner;
			cpl::CToolTipWindow tipWindow;
		};
	};
#endif