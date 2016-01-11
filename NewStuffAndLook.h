/*************************************************************************************
 
 cpl - cross-platform library - v. 0.1.0.
 
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
 
 file:ComponentContainers.h
 
	A collection of views that can contain other views.
 
 *************************************************************************************/

#ifndef _NEWSTUFFANDLOOK_H
	#define _NEWSTUFFANDLOOK_H

	#include "GraphicComponents.h"
	#include <vector>

	namespace cpl
	{


		class CDoubleClickComponent
		{


		};


		class SimpleButton
		{

		};

		template<class ButtonType = SimpleButton>
		class CRowTabBar : public CBaseControl, public juce::Component
		{
		public:

			enum ColourID
			{
				ColourSelected,
				ColourUnselected
			};

			typedef ButtonType ButtonTy;

			void setColour(ColourID id, juce::Colour colour)
			{
				if (id < ArraySize(colours))
					colours[id] = colour;
			}

			virtual void addTab(const std::string & name)
			{
				if (name.size())
					buttons.push_back(name);
			}

			virtual void paint(juce::Graphics & g) override
			{

				//g.setColour(Colours::black);
				// fill rects

				auto hoverButton = getMouseHoverButton();
				auto height = getHeight() / buttons.size();
				g.setFont(TextSize::normalText);
				//g.getCurrentFont().getHeight() / 2 + height / 2
				juce::Rectangle<int> textRectangle(5, 0, getWidth(), height);
				for (int index = 0; index < buttons.size(); ++index)
				{
					textRectangle.setY(height * index);
					auto color = selectedIndex == index ? cpl::GetColour(cpl::ColourEntry::activated) : cpl::GetColour(cpl::ColourEntry::deactivated);
					Colour textColour = (selectedIndex == index ? cpl::GetColour(cpl::ColourEntry::selfont) : cpl::GetColour(cpl::ColourEntry::auxfont));
					// enhance brightness
					if (index == hoverButton)
					{
						textColour = textColour.darker();
					}

					double pos = double(index) / (buttons.size());
					g.setColour(color);
					g.fillRect(0, pos * getHeight(), getWidth(), height);
					g.setColour(textColour);
					g.drawFittedText(buttons[index].c_str(), textRectangle, juce::Justification::centredLeft, 1);

					// 5, g.getCurrentFont().getHeight() / 2 + height/2 + height * index
					//g.drawSingleLineText(buttons[index].c_str(),);
				}
				return; // comment if you want black bars around
				g.setColour(Colours::black);
				// draw horizontal lines.
				for (int line = 1; line < buttons.size(); ++line)
				{
					double pos = double(line) / (buttons.size());
					g.drawHorizontalLine(pos * getHeight(), 0, getWidth());
				}
				if (selectedIndex != 0)
					g.drawVerticalLine(getWidth() - 1, 0, selectedIndex * height);
				if (buttons.size() > 1)
					g.drawVerticalLine(getWidth() - 1, (selectedIndex + 1) * height, getHeight());
			}

			CRowTabBar()
				: selectedIndex(0), maxTabHeight(25), CBaseControl(this), isMouseInside(false)
			{
				setRepaintsOnMouseActivity(true);
				mouseCoords[0] = mouseCoords[1] = 0;
				colours[0] = Colours::grey;
				colours[1] = Colours::lightgrey;
			}

			void setSelectedTab(int index)
			{
				if (buttons.size() > 1 && index >= 0 && index < buttons.size() && index != selectedIndex)
				{
					selectedIndex = index;
					bSetValue(double(index) / (buttons.size() - 1));
				}
			}

		protected:

			void mouseEnter(const juce::MouseEvent & e) override
			{
				isMouseInside = true;
			}

			void mouseExit(const juce::MouseEvent & e) override
			{
				isMouseInside = false;
			}

			void mouseMove(const juce::MouseEvent & e) override
			{
				mouseCoords[0] = e.x;
				mouseCoords[1] = e.y;
				if (hoverIndex != getMouseHoverButton())
					repaint();
			}

			void mouseDown(const juce::MouseEvent & e) override
			{

				mouseMove(e);
				setSelectedTab(getMouseHoverButton());
			}



			int getMouseHoverButton()
			{

				if (!isMouseInside)
					return -1;
				else
				{
					return cpl::Math::round<int>((buttons.size() - 1) * (double)mouseCoords[1] / getHeight());
				}
			}

			int selectedIndex;
			int hoverIndex;
			std::vector<std::string> buttons;

			juce::Colour colours[2];

			bool isMouseInside;
			int mouseCoords[2];
			double maxTabHeight;
		};
		template<class ButtonType = SimpleButton>
		class CTextTabBar : public CBaseControl, public juce::Component
		{
		public:

			class CTabBarListener
			{
			public:

				typedef CTextTabBar<ButtonType> BaseBarType;

				virtual void panelOpened(BaseBarType * object) {};
				virtual void panelClosed(BaseBarType * object) {};

				virtual void tabSelected(BaseBarType * object, int index) {};
				virtual ~CTabBarListener() {};
			};

			enum ColourID
			{
				ColourSelected,
				ColourUnselected
			};

			enum Type
			{
				Vertical,
				Horizontal
			};

			typedef ButtonType ButtonTy;

			void setColour(ColourID id, juce::Colour colour)
			{
				if (id < ArraySize(colours))
					colours[id] = colour;
			}

			virtual CTextTabBar & addTab(const std::string & name)
			{
				if (name.size())
				{
					buttons.push_back(name);
					renderTriangle();
				}
				return *this;
			}



			virtual void paint(juce::Graphics & g) override
			{

				//g.setColour(Colours::black);
				// fill rects



				auto hoverButton = getMouseHoverButton();
				// vertical or horizontal size
				juce::Font f("Verdana", 5, juce::Font::bold);
				g.setFont(f);
				//g.getCurrentFont().getHeight() / 2 + height / 2
				if (orientation == Vertical)
				{
					g.setFont(TextSize::normalText);
					auto size = float(getHeight()) / buttons.size();
					juce::Rectangle<int> textRectangle(5, 0, getWidth(), cpl::Math::round<int>(size - triangleSize));
					for (auto index = 0; index < (int)buttons.size(); ++index)
					{
						textRectangle.setY(cpl::Math::round<int>(size * index));
						juce::Colour textColour, color;
						if (!isIndeterminateState && selectedIndex == index)
						{
							color = cpl::GetColour(cpl::ColourEntry::activated);
							textColour = cpl::GetColour(cpl::ColourEntry::selfont);
						}
						else
						{
							color = cpl::GetColour(cpl::ColourEntry::deactivated);
							textColour = cpl::GetColour(cpl::ColourEntry::auxfont);
						}
						// enhance brightness
						if (!isIndeterminateState && index == hoverButton)
						{
							textColour = textColour.darker();
						}

						float pos = float(index) / (buttons.size());
						g.setColour(color);
						g.fillRect(0.f, pos * getHeight(), (float)getWidth(), (float)size);
						g.setColour(textColour);
						g.drawFittedText(buttons[index].c_str(), textRectangle, juce::Justification::centredLeft, 1);

						// 5, g.getCurrentFont().getHeight() / 2 + height/2 + height * index
						//g.drawSingleLineText(buttons[index].c_str(),);
					}
					return; // comment if you want black bars around
					g.setColour(Colours::black);
					// draw horizontal lines.
					for (unsigned line = 1; line < buttons.size(); ++line)
					{
						float pos = float(line) / (buttons.size());
						g.drawHorizontalLine(cpl::Math::round<int>(pos * getHeight()), (float)0, (float)getWidth());
					}
					if (selectedIndex != 0)
						g.drawVerticalLine(getWidth() - 1, 0.f, (float)selectedIndex * size);
					if (buttons.size() > 1)
						g.drawVerticalLine(getWidth() - 1, (selectedIndex + 1.f) * size, (float)getHeight());
				}
				else
				{

					//float fontSize = JUCE_LIVE_CONSTANT(11.1);
					g.setFont(TextSize::normalText);
					auto size = (float)ceil(double(getWidth()) / buttons.size());
					juce::Rectangle<int> textRectangle(cornerOffset, 0, cpl::Math::round<int>(size - triangleSize * 2.5), getHeight());
					for (auto index = 0; index < (int)buttons.size(); ++index)
					{
						textRectangle.setX(cpl::Math::round<int>(cornerOffset + size * index));
						juce::Colour textColour, color;
						if (!isIndeterminateState && selectedIndex == index)
						{
							color = cpl::GetColour(cpl::ColourEntry::activated);
							textColour = cpl::GetColour(cpl::ColourEntry::selfont);
						}
						else
						{
							color = cpl::GetColour(cpl::ColourEntry::deactivated);
							textColour = cpl::GetColour(cpl::ColourEntry::auxfont);
						}
						// enhance brightness
						if (!isIndeterminateState && index == hoverButton)
						{
							textColour = textColour.brighter(0.2f);
						}

						g.setColour(color);
						g.fillRect(size * index, 0.f, (float)size, (float)getHeight());
						g.setColour(textColour);
						g.drawFittedText(buttons[index].c_str(), textRectangle, juce::Justification::centredLeft, 1);


					}

					g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
					// draw horizontal lines.
					for (unsigned line = 0; line < (buttons.size()); ++line)
					{
						//if (line == selectedIndex)
						//g.setColour(Colours::red);
						double pos = double(line) / (buttons.size());
						g.drawLine((float)ceil(pos * getWidth()), 0.f, (float)ceil(pos * getWidth()), (float)getHeight(), 0.5f);

					}
					g.setColour(cpl::GetColour(cpl::ColourEntry::aux));
					// draw triangle
					if (isTriangleHovered)
						g.setOpacity(0.8f);
					else
						g.setOpacity(0.6f);

					if (!isIndeterminateState)
						g.fillPath(triangleVertices);

					//p.addTriangle()

					/*if (selectedIndex != 0)
					g.drawVerticalLine(getWidth() - 1, 0, selectedIndex * size);
					if (buttons.size() > 1)
					g.drawVerticalLine(getWidth() - 1, (selectedIndex + 1) * size, getHeight());*/
				}

			}
			void addListener(CTabBarListener * list)
			{
				if (list)
					listeners.push_back(list);
			}
			CTextTabBar()
				: selectedIndex(0), maxTabHeight(25), CBaseControl(this), isMouseInside(false),
				orientation(Vertical), triangleSize(5), isTriangleHovered(false), isIndeterminateState(true)
			{
				setRepaintsOnMouseActivity(true);
				mouseCoords[0] = mouseCoords[1] = 0;
				colours[0] = Colours::grey;
				
				colours[1] = Colours::lightgrey;
			}
			void setOrientation(Type orientation)
			{
				this->orientation = orientation;
				renderTriangle();
			}
			void setSelectedTab(int index)
			{
				int size = (int)buttons.size();
				if (isIndeterminateState || (size > 1 && index >= 0 && index < size && index != selectedIndex))
				{
					isIndeterminateState = false;
					selectedIndex = index;
					bSetValue(double(index) / (buttons.size() - 1));
					renderTriangle();
					for (auto & listener : listeners)
						listener->tabSelected(this, index);
				}
			}

			void openPanel()
			{
				if (panelIsClosed)
				{
					panelIsClosed = false;
					for (auto & listener : listeners)
						listener->panelOpened(this);
					renderTriangle();
					repaint();
				}
			}
			void closePanel()
			{
				if (!panelIsClosed)
				{
					panelIsClosed = true;
					for (auto & listener : listeners)
						listener->panelClosed(this);
					renderTriangle();
					repaint();
				}
			}
			bool isOpen() const
			{
				return !panelIsClosed;
			}

			int getNumTabs() const
			{
				return buttons.size();
			}

			int getSelectedTab() const
			{
				return selectedIndex;
			}

		protected:

			void resized() override
			{
				renderTriangle();
			}

			void mouseEnter(const juce::MouseEvent & e) override
			{
				isMouseInside = true;
			}

			void mouseExit(const juce::MouseEvent & e) override
			{
				isMouseInside = false;
				isTriangleHovered = false;
			}

			void mouseMove(const juce::MouseEvent & e) override
			{
				isMouseInside = true;
				mouseCoords[0] = e.x;
				mouseCoords[1] = e.y;
				auto currentHover = getMouseHoverButton();
				if (currentHover == selectedIndex)
				{
					auto tabSize = double(getWidth()) / buttons.size();
					if (e.x > (tabSize * currentHover + tabSize * 0.83))
						isTriangleHovered = true;
					else
						isTriangleHovered = false;
				}
				else
					isTriangleHovered = false;
				if (hoverIndex != currentHover)
					repaint();
			}

			void mouseDown(const juce::MouseEvent & e) override
			{
				mouseMove(e);
				if (isTriangleHovered)
				{
					panelIsClosed ? openPanel() : closePanel();
				}
				else
					setSelectedTab(getMouseHoverButton());
				mouseMove(e);
			}



			void renderTriangle()
			{
				if (!buttons.size())
					return;

				auto const offset = cornerOffset * 1.5f;

				triangleSize = float(orientation == Vertical ? getWidth() : getHeight()) - offset * 2;

				juce::Point<float> pos;
				juce::Point<float> originCenter;
				if (orientation == Vertical)
				{
					pos.setXY(offset, (1 + selectedIndex) * float(getHeight()) / buttons.size() - triangleSize - offset);
					originCenter.setXY(getWidth() / 2.0f, ((1 + selectedIndex) * float(getWidth()) / buttons.size()) - triangleSize - cornerOffset / 2.f);
				}
				else
				{
					pos.setXY((1 + selectedIndex) * float(getWidth()) / buttons.size() - triangleSize - offset, offset);
					originCenter.setXY(((1 + selectedIndex) * float(getWidth()) / buttons.size()) - triangleSize - cornerOffset / 2.f, getHeight() / 2.0f);
				}

				triangleVertices.clear();

				triangleVertices.addTriangle
				(
					pos.x + 0.f, pos.y + 0.f,
					pos.x + triangleSize, pos.y + 0.f,
					pos.x + triangleSize / 2, pos.y + triangleSize
				);

				if (orientation == Horizontal)
				{
					if (panelIsClosed)
					{
						triangleVertices.applyTransform(AffineTransform::identity.rotated((float)M_PI / 2, originCenter.getX(), originCenter.getY()));
						//triangleVertices.applyTransform(AffineTransform::identity.rotated(rotation, originCenter.getX(), originCenter.getY()));
					}
					else
					{

					}
				}
				else
				{
					if (panelIsClosed)
					{
						triangleVertices.applyTransform(AffineTransform::identity.rotated((float)M_PI / 2, originCenter.getX(), originCenter.getY()));
					}
					else
					{
						triangleVertices.applyTransform(AffineTransform::identity.rotated((float)-M_PI, originCenter.getX(), originCenter.getY()));
					}
				}

				//triangleVertices.applyTransform(AffineTransform::identity.rotated(rotation, originCenter.getX(), originCenter.getY()));

			}
			int getMouseHoverButton()
			{

				if (!isMouseInside)
					return -1;
				else
				{
					double fraction;
					if (orientation == Vertical)
						fraction = double(mouseCoords[1]) / getHeight();
					else
						fraction = double(mouseCoords[0]) / getWidth();
					return std::min<int>(int(buttons.size() * fraction), (int)buttons.size() - 1);
				}
			}
			static const int cornerOffset = 5;
			int selectedIndex;
			int hoverIndex;
			std::vector<std::string> buttons;
			// tabs at start doesn't have a selected index.
			bool isIndeterminateState;
			juce::Colour colours[2];
			Type orientation;
			bool isMouseInside;
			int mouseCoords[2];
			double maxTabHeight;
			bool isTriangleHovered;
			bool panelIsClosed;
			float triangleSize;
			juce::Path triangleVertices;
			std::vector<CTabBarListener*> listeners;
		};

		class CTriangleButton : public juce::Button
		{
		public:

			enum Direction
			{
				left, top, right, bottom
			};

			CTriangleButton()
				: juce::Button("TriangleButton"), hoverBrightness(0.2f)
			{
				dirs[0] = Direction::left;
				dirs[1] = Direction::bottom;
				colours[0] = colours[1] = cpl::GetColour(ColourEntry::aux);
				setClickingTogglesState(true);
				setOpaque(false);
			}

			void setHoverBrightness(float newVal)
			{
				hoverBrightness = cpl::Math::confineTo(newVal, -1.f, 1.f);
			}

			void setActivatedDirection(Direction dir) { dirs[1] = dir; }
			void setDeactivatedDirection(Direction dir) { dirs[0] = dir; }
			void setActivatedColour(juce::Colour colour) { colours[1] = colour;	}
			void setDeactivatedColour(juce::Colour colour) { colours[0] = colour;}

			void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
			{
				g.setColour(colours[isButtonDown].withMultipliedBrightness(1.f + isMouseOverButton * hoverBrightness));
				g.fillPath(triangleVertices);
			}
			

			void clicked() override
			{
				renderTriangle();
			}

			void resized() override
			{
				renderTriangle();
			}

			void renderTriangle()
			{
				triangleVertices.clear();

				triangleVertices.addTriangle
				(
					(float)getWidth(), (float)0,
					(float)getWidth(), (float)getHeight(),
					(float)0, getHeight() * 0.5f
				);

				triangleVertices.applyTransform(
					AffineTransform::identity.rotated(dirs[getToggleState()] * float(M_PI * 0.5), getWidth() * 0.5f, getHeight() * 0.5f)
				);
			}

		private:
			Direction dirs[2];
			juce::Colour colours[2];
			juce::Path triangleVertices;
			float hoverBrightness;
		};


		class CIconTabBar : public CTextTabBar<>
		{
		public:
			virtual void paint(juce::Graphics & g) override
			{

				//g.setColour(Colours::black);
				// fill rects

				auto cornerOffset = 5;

				auto hoverButton = getMouseHoverButton();
				auto height = (getHeight()) / (int)buttons.size();
				auto iconHeight = height - cornerOffset * 2;
				//g.getCurrentFont().getHeight() / 2 + height / 2
				juce::Rectangle<int> iconRectangle(cornerOffset, cornerOffset, getWidth() - cornerOffset * 2, iconHeight);
				for (int index = 0; index < (int)buttons.size(); ++index)
				{
					iconRectangle.setY(height * index + cornerOffset);
					bool isSelectedIndex = selectedIndex == index;

					auto color = isSelectedIndex ? 
						cpl::GetColour(cpl::ColourEntry::activated) : cpl::GetColour(cpl::ColourEntry::deactivated);
					Colour textColour = (isSelectedIndex ? cpl::GetColour(cpl::ColourEntry::selfont) : cpl::GetColour(cpl::ColourEntry::auxfont));
					// enhance brightness
					if (index == hoverButton)
					{
						color = color.brighter(0.05f);
					}

					double pos = double(index) / (buttons.size());
					g.setColour(color);
					g.fillRect(0, cpl::Math::round<int>(pos * getHeight()), getWidth(), height);

					//auto const & image = vectors[index].getImage();
					g.setOpacity(isSelectedIndex ? 1 : 0.5f);
					g.drawImageAt(vectors[(unsigned)index].getImage(), cornerOffset, height * index + cornerOffset);
					g.setOpacity(1.f);
					/*g.drawImage
					(
					image,
					iconRectangle.getX(), iconRectangle.getY(), iconRectangle.getWidth(), iconRectangle.getHeight(),
					0, 0, image.getWidth(), image.getHeight()
					);*/
					//g.drawFittedText(buttons[index].c_str(), textRectangle, juce::Justification::centredLeft, 1);

					// 5, g.getCurrentFont().getHeight() / 2 + height/2 + height * index
					//g.drawSingleLineText(buttons[index].c_str(),);
				}
				//return; // comment if you want black bars around
				g.setColour(cpl::GetColour(cpl::ColourEntry::separator));
				// draw horizontal lines.
				//return;
				for (unsigned line = 0; line < buttons.size(); ++line)
				{
					double pos = double(line) / (buttons.size());
					g.drawLine(0.f, float(pos * getHeight()), (float)getWidth(), float(pos * getHeight()), 0.5f);
				}
				if (selectedIndex != 0)
					g.drawLine((float)getWidth() - 1, (float)0, (float)getWidth() - 1, (float)selectedIndex * height, 0.5f);
				if (buttons.size() > 1)
					g.drawLine((float)getWidth() - 1, (float)(selectedIndex + 1) * height, (float)getWidth() - 1, (float)getHeight(), 0.5f);
			}

			void resized() override
			{
				if (!buttons.size())
					return;
				if (!getWidth() || !getHeight())
					return;
				//auto hoverButton = getMouseHoverButton();

				auto height = (getHeight()) / (int)buttons.size();
				auto iconHeight = height - cornerOffset * 2;
				//g.getCurrentFont().getHeight() / 2 + height / 2
				juce::Rectangle<float> iconRectangle((float)cornerOffset, (float)cornerOffset, (float)getWidth() - cornerOffset * 2.0f, (float)iconHeight);

				for (auto & vec : vectors)
					vec.renderImage(iconRectangle.withZeroOrigin(), cpl::GetColour(cpl::ColourEntry::selfont));

			}

			CTextTabBar<> & addTab(const std::string & name)
			{
				if (name.size())
				{
					if (cpl::contains(buttons, name))
						CPL_RUNTIME_EXCEPTION("Multiple tabs with same unique name!");
					buttons.push_back(name);
					vectors.emplace_back(name);
					resized();
				}
				return *this;
			}


		private:
			std::vector<CVectorResource> vectors;

		};

		class NameComp : public juce::Component
		{
			cpl::CIconTabBar bar;
			cpl::CComboBox box;
		public:
			NameComp(std::string name, bool hasIcons = true)
				: content(name), sel(sel.showColourspace | sel.showColourAtTop, 2, 2), hasIcons(hasIcons),
				knob("SomeValue", knob.hz), color("smukt")
			{
				knob.bToggleEditSpaces(true);
				knob.bSetDescription("This is a general knob.");
				knob.bSetPos(10, 10);
				if (hasIcons)
				{
					bar.setOrientation(bar.Vertical);
					bar.addTab("icons/svg/gear.svg");
					bar.addTab("icons/svg/painting.svg");
					bar.addTab("icons/svg/wrench.svg");
					bar.addTab("icons/svg/formulae.svg");
					addAndMakeVisible(bar);
				}
				else
				{
					addAndMakeVisible(knob);
					addAndMakeVisible(box);
				}
				box.bSetTitle("Algorithm:");
				box.bSetPos(150, 5);

				box.setValues("Fast Fourier Transform|FFT|Minimum Q DFT|Resonators");


				color.bSetPos(300, 20);
				color.bToggleEditSpaces(true);
				addAndMakeVisible(color);
				//addAndMakeVisible(sel);
			}

			void paint(Graphics & g)
			{
				g.fillAll(cpl::GetColour(cpl::ColourEntry::activated));
				g.setFont(TextSize::largeText);
				g.setColour(cpl::GetColour(cpl::ColourEntry::selfont));
				g.drawFittedText(content, getBounds(), Justification::centredRight, 1);
				if (hasIcons)
				{
					g.setFont(cpl::TextSize::smallerText);
					g.drawText("Lazy dog caught the quick fox.", juce::Rectangle<float>(35, 5, 300, 20), juce::Justification::topLeft, true);
					g.setFont(cpl::TextSize::smallText);
					g.drawText("Lazy dog caught the quick fox.", juce::Rectangle<float>(35, 20, 300, 20), juce::Justification::topLeft, true);
					g.setFont(cpl::TextSize::normalText);
					g.drawText("Lazy dog caught the quick fox.", juce::Rectangle<float>(35, 40, 300, 20), juce::Justification::topLeft, true);
					g.setFont(cpl::TextSize::largeText);
					g.drawText("Lazy dog caught the quick fox.", juce::Rectangle<float>(35, 70, 300, 20), juce::Justification::topLeft, true);
				}
			}

			void resized()
			{
				bar.setBounds(0, 0, 25, getHeight());

			}
			std::string content;
			juce::ColourSelector sel;
			bool hasIcons;
			CKnobSlider knob;
			CColourControl color;
		};

		class CSVGButton : public juce::Button, public cpl::CBaseControl
		{
		public:
			CSVGButton()
				: Button("IconButton"), CBaseControl(this), pst(1.f)
			{
				setClickingTogglesState(true);
				enableTooltip(true);
				addListener(this);
			}

			void setImage(std::string imagePath)
			{
				rsc.associate(imagePath);
				resized();
			}

			void resized() override
			{
				auto size = juce::Rectangle<int>(0, 0, getBounds().getWidth() - 2 * cornerOffset, getBounds().getHeight() - 2 * cornerOffset);
				rsc.renderImage<int>(size, cpl::GetColour(ColourEntry::selfont));

				rect.clear();
				rect.addRectangle(0.f, 0.f, (float)getWidth() - 1, (float)getHeight() - 1);
			}

			iCtrlPrec_t bGetValue() const override
			{
				return getToggleState() ? 1.0f : 0.f;
			}

			void bSetValue(iCtrlPrec_t val, bool sync = false) override
			{
				setToggleState(val > 0.5, 
					sync ? juce::NotificationType::sendNotificationSync : juce::NotificationType::sendNotification );
			}

			void bSetInternal(iCtrlPrec_t val) override
			{
				setToggleState(val > 0.5, juce::NotificationType::dontSendNotification);
			}

			void paintButton(juce::Graphics & g, bool isMouseOverButton, bool isButtonDown) override
			{

				if (getToggleState() && !isButtonDown)
				{
					g.fillAll(cpl::GetColour(ColourEntry::activated));
				}
				else if (isButtonDown)
				{
					g.fillAll(cpl::GetColour(ColourEntry::deactivated).brighter(0.3f));
				}
				else if (isMouseOverButton)
				{
					g.fillAll(cpl::GetColour(ColourEntry::deactivated).brighter(0.1f));
				}
				else
				{
					g.fillAll(cpl::GetColour(ColourEntry::deactivated));
				}

				if (getToggleState())
				{
					g.setOpacity(1.f);
				}

				else
				{
					g.setOpacity(0.5f);
				}


				g.drawImageAt(rsc.getImage(), cornerOffset, cornerOffset);

				g.setColour(getToggleState() ? cpl::GetColour(ColourEntry::deactivated) : cpl::GetColour(ColourEntry::separator));
				//g.strokePath(rect, pst);
			}

		private:
			static const int cornerOffset = 4;
			CVectorResource rsc;
			juce::Path rect;
			juce::PathStrokeType pst;
		};
	};

#endif