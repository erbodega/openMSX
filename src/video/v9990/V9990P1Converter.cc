// $Id$

#include "V9990P1Converter.hh"
#include "V9990.hh"
#include "V9990VRAM.hh"
#include "GLUtil.hh"
#include <cassert>
#include <algorithm>

namespace openmsx {

#ifdef COMPONENT_GL
// On some systems, "GLuint" is not equivalent to "unsigned int",
// so BitmapConverter must be instantiated separately for those systems.
// But on systems where it is equivalent, it's an error to expand
// the same template twice.
// The following piece of template metaprogramming expands
// V9990BitmapConverter<GLuint, Renderer::ZOOM_REAL> to an empty class if
// "GLuint" is equivalent to "unsigned int"; otherwise it is expanded to
// the actual V9990BitmapConverter implementation.
//
// See BitmapConverter.cc
class NoExpansion {};
// ExpandFilter::ExpandType = (Type == unsigned int ? NoExpansion : Type)
template <class Type> class ExpandFilter {
	typedef Type ExpandType;
};
template <> class ExpandFilter<unsigned> {
	typedef NoExpansion ExpandType;
};
template <> class V9990P1Converter<NoExpansion> {};
template class V9990P1Converter<ExpandFilter<GLuint>::ExpandType>;
#endif // COMPONENT_GL

template <class Pixel>
V9990P1Converter<Pixel>::V9990P1Converter(V9990& vdp_, Pixel* palette64_)
	: vdp(vdp_), vram(vdp.getVRAM())
	, palette64(palette64_)
{
}

template <class Pixel>
void V9990P1Converter<Pixel>::convertLine(Pixel* linePtr,
		unsigned displayX, unsigned displayWidth, unsigned displayY)
{
	unsigned prioX = vdp.getPriorityControlX();
	unsigned prioY = vdp.getPriorityControlY();

	int visibleSprites[16 + 1];
	determineVisibleSprites(visibleSprites, displayY);

	if (displayY > prioY) prioX = 0;

	unsigned displayAX = (displayX + vdp.getScrollAX()) & 511;
	unsigned displayBX = (displayX + vdp.getScrollBX()) & 511;

	// TODO check roll behaviour
	unsigned rollMask = vdp.getRollMask(0x1FF);
	unsigned scrollAY = vdp.getScrollAY();
	unsigned scrollBY = vdp.getScrollBY();
	unsigned scrollAYBase = scrollAY & ~rollMask;
	unsigned scrollBYBase = scrollBY & ~rollMask;
	unsigned displayAY = scrollAYBase + (displayY + scrollAY) & rollMask;
	unsigned displayBY = scrollBYBase + (displayY + scrollBY) & rollMask;

	unsigned displayEnd = displayX + displayWidth;
	unsigned end = std::min(prioX, displayEnd);

	byte offset = vdp.getPaletteOffset();
	byte palA = (offset & 0x03) << 4;
	byte palB = (offset & 0x0C) << 2;

	for (/* */; displayX < end; ++displayX) {
		Pixel pix = raster(displayAX, displayAY, 0x7C000, 0x00000, palA, // A
		                   displayBX, displayBY, 0x7E000, 0x40000, palB, // B
		                   visibleSprites, displayX, displayY);
		*linePtr++ = pix;

		displayAX = (displayAX + 1) & 511;
		displayBX = (displayBX + 1) & 511;
	}
	for (/* */; displayX < displayEnd; ++displayX) {
		Pixel pix = raster(displayBX, displayBY, 0x7E000, 0x40000, palB, // B
		                   displayAX, displayAY, 0x7C000, 0x00000, palA, // A
		                   visibleSprites, displayX, displayY);
		*linePtr++ = pix;

		displayAX = (displayAX + 1) & 511;
		displayBX = (displayBX + 1) & 511;
	}
}

template <class Pixel>
Pixel V9990P1Converter<Pixel>::raster(
	unsigned xA, unsigned yA, unsigned nameA, unsigned patternA, byte palA,
	unsigned xB, unsigned yB, unsigned nameB, unsigned patternB, byte palB,
	int* visibleSprites, unsigned int x, unsigned int y)
{
	byte p;
	if (vdp.spritesEnabled()) {
		// Front sprite plane
		p = getSpritePixel(visibleSprites, x, y, true);

		if (!(p & 0x0F)) {
			// Front image plane
			p = getPixel(xA, yA, nameA, patternA) + palA;

			if (!(p & 0x0F)) {
				// Back sprite plane
				p = getSpritePixel(visibleSprites, x, y, false);

				if (!(p & 0x0F)) {
					// Back image plane
					p = getPixel(xB, yB, nameB, patternB) + palB;

					if (!(p & 0x0F)) {
						// Backdrop color
						p = vdp.getBackDropColor();
					}
				}
			}
		}
	} else {
		// Front image plane
		p = getPixel(xA, yA, nameA, patternA) + palA;

		if (!(p & 0x0F)) {
			// Back image plane
			p = getPixel(xB, yB, nameB, patternB) + palB;

			if (!(p & 0x0F)) {
				// Backdrop color
				p = vdp.getBackDropColor();
			}
		}
	}
	return palette64[p];
}

template <class Pixel>
byte V9990P1Converter<Pixel>::getPixel(
	unsigned x, unsigned y, unsigned nameTable, unsigned patternTable)
{
	unsigned address = nameTable + (((y / 8) * 64 + (x / 8)) * 2);
	unsigned pattern = (vram.readVRAMP1(address + 0) +
	                        vram.readVRAMP1(address + 1) * 256) & 0x1FFF;
	unsigned x2 = (pattern % 32) * 8 + (x % 8);
	unsigned y2 = (pattern / 32) * 8 + (y % 8);
	address = patternTable + y2 * 128 + x2 / 2;
	byte dixel = vram.readVRAMP1(address);
	if (!(x & 1)) dixel >>= 4;
	return dixel & 0x0F;
}

template <class Pixel>
void V9990P1Converter<Pixel>::determineVisibleSprites(
	int* visibleSprites, unsigned displayY)
{
	static const unsigned spriteTable = 0x3FE00;

	int index = 0;
	for (unsigned sprite = 0; sprite < 125; ++sprite) {
		unsigned spriteInfo = spriteTable + 4 * sprite;
		byte attr = vram.readVRAMP1(spriteInfo + 3);

		if (!(attr & 0x10)) {
			byte spriteY = vram.readVRAMP1(spriteInfo) + 1;
			byte posY = displayY - spriteY;
			if (posY < 16) {
				visibleSprites[index++] = sprite;
				if (index == 16) break;
			}
		}
	}
	visibleSprites[index] = -1;
}

template <class Pixel>
byte V9990P1Converter<Pixel>::getSpritePixel(
	int* visibleSprites, unsigned x, unsigned y, bool front)
{
	static const unsigned spriteTable = 0x3FE00;
	int spritePatternTable = vdp.getSpritePatternAddress(P1);

	for (unsigned sprite = 0; visibleSprites[sprite] != -1; ++sprite) {
		unsigned addr     = spriteTable + 4 * visibleSprites[sprite];
		unsigned spriteX  = vram.readVRAMP1(addr + 2);
		byte  spriteAttr = vram.readVRAMP1(addr + 3);
		spriteX += 256 * (spriteAttr & 0x03);
		if (spriteX > 1008) spriteX -= 1024; // hack X coord into -16..1008

		unsigned posX = x - spriteX;
		if ((posX < 16) && (front ^ !!(spriteAttr & 0x20))) {
			byte spriteY  = vram.readVRAMP1(addr + 0);
			byte spriteNo = vram.readVRAMP1(addr + 1);
			spriteY = y - (spriteY + 1);
			addr = spritePatternTable
			     + (128 * ((spriteNo & 0xF0) + spriteY))
			     + (  8 *  (spriteNo & 0x0F))
			     + (posX / 2);
			byte dixel = vram.readVRAMP1(addr);
			if (!(posX & 1)) dixel >>= 4;
			dixel &= 0x0F;
			if (dixel) {
				return dixel | ((spriteAttr >> 2) & 0x30);
			}
		}
	}
	return 0;
}


// Force template instantiation
template class V9990P1Converter<word>;
template class V9990P1Converter<unsigned>;

} // namespace openmsx
