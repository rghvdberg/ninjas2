/*
 * Copyright (C) 2018-2019 Rob van den Berg <rghvdberg at gmail dot org>
 *
 * This file is part of Ninjas2
 *
 * Nnjas2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ninjas2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ninjas2.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WOLF_LABEL_BOX_HPP_INCLUDED
#define WOLF_LABEL_BOX_HPP_INCLUDED

#include <string>
#include "Widget.hpp"
#include "NanoVG.hpp"

START_NAMESPACE_DISTRHO

class LabelBox : public NanoWidget
{
  public:
    explicit LabelBox(NanoWidget *widget, Size<uint> size) noexcept;
    explicit LabelBox(Window &parent, Size<uint> size) noexcept;

    void setFontSize(float fontSize);
    float getFontSize();

    void setText(const std::string text);
    const std::string getText();

    void setFontId(NanoVG::FontId fontId);
    NanoVG::FontId getFontId();

    void setBoxColor(const Color color);
    void setBorderColor(const Color color);
    void setTextColor(const Color color);

  protected:
    void onNanoDisplay() override;

  private:
    std::string fText;
    float fFontSize;
    NanoVG::FontId fFontId;
    Color boxColor, borderColor, textColor;
    DISTRHO_LEAK_DETECTOR(LabelBox)
};

END_NAMESPACE_DISTRHO

#endif
