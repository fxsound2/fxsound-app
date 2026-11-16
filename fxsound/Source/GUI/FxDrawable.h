/*
FxSound
Copyright (C) 2026  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FXDRAWABLE_H
#define FXDRAWABLE_H

#include <JuceHeader.h>

// JUCE 9.0.0: Drawable no longer derives from Component and must be wrapped in a DrawableComponent.
// Older JUCE (6.x / 8.x) is still used for builds that must support Windows 7.
#define FX_JUCE_DRAWABLE_IS_COMPONENT (JUCE_VERSION < 0x090000)

//==============================================================================
/*
	Owns a Drawable together with the Component that displays it, on any JUCE version.
	Before JUCE 9 the Drawable is itself the Component; from JUCE 9 it is wrapped in a DrawableComponent.
*/
class FxDrawable
{
public:
#if FX_JUCE_DRAWABLE_IS_COMPONENT
	using ComponentType = Drawable;
#else
	using ComponentType = DrawableComponent;
#endif

	FxDrawable() = default;
	~FxDrawable() = default;

	void reset(std::unique_ptr<Drawable> drawable = nullptr);

	ComponentType* get() const noexcept;
	ComponentType* operator->() const noexcept { return get(); }
	explicit operator bool() const noexcept { return get() != nullptr; }

private:
	std::unique_ptr<Drawable> drawable_;
#if ! FX_JUCE_DRAWABLE_IS_COMPONENT
	std::unique_ptr<DrawableComponent> component_; // references drawable_, so declared after it to be destroyed first
#endif

	JUCE_DECLARE_NON_COPYABLE(FxDrawable)
};

#endif
