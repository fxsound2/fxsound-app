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

#include "FxDrawable.h"

void FxDrawable::reset(std::unique_ptr<Drawable> drawable)
{
#if FX_JUCE_DRAWABLE_IS_COMPONENT
	drawable_ = std::move(drawable);
#else
	component_.reset(); // references drawable_, so must go first
	drawable_ = std::move(drawable);
	if (drawable_ != nullptr)
	{
		// drawableBounds matches how Drawable sized itself before JUCE 9
		component_ = std::make_unique<DrawableComponent>(*drawable_, BoundsToEnclose::drawableBounds);
	}
#endif
}

FxDrawable::ComponentType* FxDrawable::get() const noexcept
{
#if FX_JUCE_DRAWABLE_IS_COMPONENT
	return drawable_.get();
#else
	return component_.get();
#endif
}
