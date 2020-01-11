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

#include "LabelBox.hpp"
#include "Mathf.hpp"

START_NAMESPACE_DISTRHO

LabelBox::LabelBox(NanoWidget *widget, Size<uint> size) noexcept : NanoWidget(widget)
{
    setSize(size);
    loadSharedResources();
   
}
LabelBox::LabelBox(Window &parent,  Size<uint> size) noexcept : NanoWidget(parent)
{
    setSize(size);
    loadSharedResources();
    
}

void LabelBox::onNanoDisplay()
{
    const float width = getWidth();
    const float height = getHeight();
    const float verticalMargin = 6.0f;
    const float boxOutlineWidth = 1.0f;

    //Box background
    beginPath();

    fillColor(boxColor);
    strokeColor(borderColor);
    strokeWidth(boxOutlineWidth);

    roundedRect(boxOutlineWidth, boxOutlineWidth, width - 2 * boxOutlineWidth, height - 2 * boxOutlineWidth, 4.0f);
    fill();
    stroke();

    closePath();
    
    //Text
    beginPath();

    fontFace("__dpf_dejavusans_ttf__");
    fontSize(fFontSize);
    fillColor(textColor);
    textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
    printf("getText() = %s\n",getText().c_str());
    text(4, std::round(height / 2.0f + verticalMargin / 2.0f - 2), fText.c_str(), NULL);
    closePath();
}

void LabelBox::setFontSize(float fontSize)
{
    fFontSize = fontSize;
}

float LabelBox::getFontSize()
{
    return fFontSize;
}

void LabelBox::setText(const std::string text)
{
    fText = text;
}

const std::string LabelBox::getText()
{
    return fText;
}

void LabelBox::setFontId(NanoVG::FontId fontId)
{
    fFontId = fontId;
}

NanoVG::FontId LabelBox::getFontId()
{
    return fFontId;
}

void LabelBox::setBoxColor(const Color color)
{
    boxColor = color;
}
void LabelBox::setBorderColor(const Color color)
{
    borderColor = color;
}
void LabelBox::setTextColor(const Color color)
{
    textColor = color;
}

END_NAMESPACE_DISTRHO