/*
Copyright (c) 2009 Peter "Corsix" Cawley

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CORSIX_TH_TH_GFX_H_
#define CORSIX_TH_TH_GFX_H_
#include "th.h"
#include "th_gfx_dx9.h"
#include "th_gfx_ogl.h"
#include "th_gfx_sdl.h"
#include <stddef.h>

#ifndef CORSIX_TH_HAS_RENDERING_ENGINE
#error No rendering engine enabled in config file
#endif

//! Bitflags for drawing operations
enum THDrawFlags
{
    /** Sprite drawing flags **/
    /* Where possible, designed to be the same values used by TH data files */

    //! Draw with the left becoming the right and vice versa
    THDF_FlipHorizontal = 1 <<  0,
    //! Draw with the top becoming the bottom and vice versa
    THDF_FlipVertical   = 1 <<  1,
    //! Draw with 50% transparency
    THDF_Alpha50        = 1 <<  2,
    //! Draw with 75% transparency
    THDF_Alpha75        = 1 <<  3,
    //! Draw using a remapped palette
    THDF_AltPalette     = 1 <<  4,

    /** Object attached to tile flags **/
    /* (should be set prior to attaching to a tile) */

    //! Attach to the early sprite list (right-to-left pass)
    THDF_EarlyList      = 1 << 10,
    //! Keep this sprite at the bottom of the attached list
    THDF_ListBottom     = 1 << 11,
};

//! Bitflags for animation frames
enum THFrameFlags
{
    //! First frame of an animation
    THFF_AnimationStart = 1 << 0,
};

struct THRenderTargetCreationParams
{
    int iWidth;
    int iHeight;
    int iBPP;
    uint32_t iSDLFlags;
    bool bHardware;
    bool bDoubleBuffered;
    bool bFullscreen;
    bool bPresentImmediate;
    bool bReuseContext;
};

/*!
    Base class for a linked list of drawable objects.
    Note that "object" is used as a generic term, not in specific reference to
    game objects (though they are the most common thing in drawing lists).
*/
struct THDrawable : public THLinkList
{
    //! Draw the object at a specific point on a render target
    void (*fnDraw)(THDrawable* pSelf, THRenderTarget* pCanvas, int iDestX, int iDestY);

    //! Perform a hit test against the object
    /*!
        Should return true if when the object is drawn at (iDestX, iDestY) on a canvas,
        the point (iTestX, iTestY) is within / on the object.
    */
    bool (*fnHitTest)(THDrawable* pSelf, int iDestX, int iDestY, int iTestX, int iTestY);

    //! Drawing flags (zero or more list flags from THDrawFlags)
    unsigned long iFlags;
};

/*!
    Utility class for decoding Theme Hospital "chunked" graphics files.
    Generally used internally by THSpriteSheet.
*/
class THChunkRenderer
{
public:
    //! Initialise a renderer for a specific size result
    /*!
        @param width Pixel width of the resulting image
        @param height Pixel height of the resulting image
        @param buffer If NULL, then a new buffer is created to render the image
          onto. Otherwise, should be an array at least width*height in size.
          Ownership of this pointer is assumed by the class - call takeData()
          to take ownership back again.
    */
    THChunkRenderer(int width, int height, unsigned char *buffer = NULL);

    ~THChunkRenderer();

    //! Convert a stream of chunks into a raw bitmap
    /*!
        @param bComplex true if pData is a stream of "complex" chunks, false if
          pData is a stream of "simple" chunks. Passing the wrong value will
          usually result in a very visible wrong result.

        Use getData() or takeData() to obtain the resulting bitmap.
    */
    void decodeChunks(const unsigned char* pData, int iDataLen, bool bComplex);

    //! Get the result buffer, and take ownership of it
    /*!
        This transfers ownership of the buffer to the caller. After calling,
        the class will not have any buffer, and thus cannot be used for
        anything.
    */
    unsigned char* takeData();

    //! Get the result buffer
    inline const unsigned char* getData() const {return m_data;}

    //! Perform a "copy" chunk (normally called by decodeChunks)
    void chunkCopy(int npixels, const unsigned char* data);

    //! Perform a "fill" chunk (normally called by decodeChunks)
    void chunkFill(int npixels, unsigned char value);

    //! Perform a "fill to end of line" chunk (normally called by decodeChunks)
    void chunkFillToEndOfLine(unsigned char value);

    //! Perform a "fill to end of file" chunk (normally called by decodeChunks)
    void chunkFinish(unsigned char value);

protected:
    inline bool _isDone() {return m_ptr == m_end;}
    inline void _fixNpixels(int& npixels) const;
    inline void _incrementPosition(int npixels);

    unsigned char *m_data, *m_ptr, *m_end;
    int m_x, m_y, m_width, m_height;
    bool m_skip_eol;
};

/*!
    Utility class for using a sprite sheet as a font.
*/
class THFont
{
public:
    THFont();

    //! Set the character glyph sprite sheet
    /*!
        The sprite sheet should have the space character (ASCII 0x20) at sprite
        index 1, and other ASCII characters following on in simple order (i.e.
        '!' (ASCII 0x21) at index 2, 'A' (ASCII 0x41) at index 34, etc.)
    */
    void setSpriteSheet(THSpriteSheet* pSpriteSheet);

    //! Set the seperation between characters and between lines
    /*!
        Generally, the sprite sheet glyphs will already include separation, and
        thus no extra separation is required (set iCharSep and iLineSep to 0).
    */
    void setSeparation(int iCharSep, int iLineSep);

    //! Get the size of a single line of text
    void getTextSize(const char* sMessage, size_t iMessageLength, int* pX, int* pY) const;

    //! Draw a single line of text
    void drawText(THRenderTarget* pCanvas, const char* sMessage, size_t iMessageLength, int iX, int iY) const;

    //! Draw a single line of text, splitting it at word boundaries
    /*!
        This function still only draws a single line of text (i.e. any line
        breaks like \r and \n in sMessage are ignored), but inserts line breaks
        between words so that no single line is wider than iWidth pixels.
    */
    void drawTextWrapped(THRenderTarget* pCanvas, const char* sMessage, size_t iMessageLength, int iX, int iY, int iWidth) const;

protected:
    THSpriteSheet* m_pSpriteSheet;
    int m_iCharSep;
    int m_iLineSep;
};

//! Layer information (see THAnimationManager::drawFrame)
struct THLayers_t
{
    unsigned char iLayerContents[13];
};

//! Theme Hospital sprite animation manager
/*!
    An animation manager takes a sprite sheet and four animation information
    files, and uses them to draw animation frames and provide information about
    the animations.
*/
class THAnimationManager
{
public:
    THAnimationManager();
    ~THAnimationManager();

    void setSpriteSheet(THSpriteSheet* pSpriteSheet);

    //! Load animation information
    /*!
        setSpriteSheet() must be called before calling this.
        @param pStartData Animation first frame indicies (e.g. VSTART-1.ANI)
        @param pFrameData Frame details (e.g. VFRA-1.ANI)
        @param pListData Element indicies list (e.g. VLIST-1.ANI)
        @param pElementData Element details (e.g. VELE-1.ANI)
    */
    bool loadFromTHFile(const unsigned char* pStartData, size_t iStartDataLength,
                        const unsigned char* pFrameData, size_t iFrameDataLength,
                        const unsigned char* pListData, size_t iListDataLength,
                        const unsigned char* pElementData, size_t iElementDataLength);

    //! Get the total numer of animations
    unsigned int getAnimationCount() const;

    //! Get the total number of animation frames
    unsigned int getFrameCount() const;

    //! Get the index of the first frame of an animation
    unsigned int getFirstFrame(unsigned int iAnimation) const;

    //! Get the index of the frame after a given frame
    /*!
        To draw an animation frame by frame, call getFirstFrame() to get the
        index of the first frame, and then keep on calling getNextFrame() using
        the most recent return value from getNextFrame() or getFirstFrame().
    */
    unsigned int getNextFrame(unsigned int iFrame) const;

    //! Set the palette remap data for an animation
    /*!
        This sets the palette remap data for every single sprite used by the
        given animation. If the animation (or any of its sprites) are drawn
        using the THDF_AltPalette flag, then palette indicies will be mapped to
        new palette indicies by the 256 byte array pMap. This is typically used
        to draw things in different colours or in greyscale.
    */
    void setAnimationAltPaletteMap(unsigned int iAnimation, const unsigned char* pMap);

    //! Draw an animation frame
    /*!
        @param pCanvas The render target to draw onto.
        @param iFrame The frame index to draw (should be in range [0, getFrameCount() - 1])
        @param oLayers Information to decide what to draw on each layer.
            An animation is comprised of upto thirteen layers, numbered 0
            through 12. Some animations will have different options for what to
            render on each layer. For example, patient animations generally
            have the different options on layer 1 as different clothes, so if
            layer 1 is set to the value 0, they may have their default clothes,
            and if set to the value 2 or 4 or 6, they may have other clothes.
            Play with the AnimView tool for a better understanding of layers,
            though note that while it can draw more than one option on each
            layer, this class can only draw a single option for each layer.
        @param iX The screen position to use as the animation X origin.
        @param iY The screen position to use as the animation Y origin.
        @param iFlags Zero or more THDrawFlags flags.
    */
    void drawFrame(THRenderTarget* pCanvas, unsigned int iFrame, const THLayers_t& oLayers, int iX, int iY, unsigned long iFlags) const;

    bool hitTest(unsigned int iFrame, const THLayers_t& oLayers, int iX, int iY, unsigned long iFlags, int iTestX, int iTestY) const;

protected:
#pragma pack(push)
#pragma pack(1)
    struct th_anim_t
    {
        uint16_t frame;
        // It could be that frame is a uint32_t rather than a uint16_t, which
        // would resolve the following unknown (which seems to always be zero).
        uint16_t unknown;
    };

    struct th_frame_t
    {
        uint32_t list_index;
        // These fields have something to do with width and height, but it's
        // not clear quite exactly how.
        uint8_t width;
        uint8_t height;
        // If non-zero, index into sound.dat filetable.
        uint8_t sound;
        // Combination of zero or more THFrameFlags values
        uint8_t flags;
        uint16_t next;
    };

    struct th_element_t
    {
        uint16_t table_position;
        uint8_t offx;
        uint8_t offy;
        // High nibble: The layer which the element belongs to [0, 12]
        // Low  nibble: Zero or more THDrawFlags flags
        uint8_t flags;
        // The layer option / layer id
        uint8_t layerid;
    };
#pragma pack(pop)

    struct frame_t
    {
        unsigned int iListIndex;
        unsigned int iNextFrame;
        unsigned int iSound;
        unsigned int iFlags;
        // Bounding rectangle is with all layers / options enabled - used as a
        // quick test prior to a full pixel perfect test.
        int iBoundingLeft;
        int iBoundingRight;
        int iBoundingTop;
        int iBoundingBottom;
    };

    struct element_t
    {
        unsigned int iSprite;
        unsigned int iFlags;
        int iX;
        int iY;
        unsigned char iLayer;
        unsigned char iLayerId;
    };

    unsigned int* m_pFirstFrames;
    frame_t* m_pFrames;
    uint16_t* m_pElementList;
    element_t* m_pElements;
    THSpriteSheet* m_pSpriteSheet;

    unsigned int m_iAnimationCount;
    unsigned int m_iFrameCount;
    unsigned int m_iElementCount;
};

class THAnimation : protected THDrawable
{
public:
    THAnimation();

    void removeFromTile();
    void attachToTile(THLinkList *pMapNode);

    void tick();
    void draw(THRenderTarget* pCanvas, int iDestX, int iDestY);
    bool hitTest(int iDestX, int iDestY, int iTestX, int iTestY);

    THLinkList* getPrevious() {return this->pPrev;}
    unsigned long getFlags() {return this->iFlags;}
    unsigned int getAnimation() {return m_iAnimation;}
    int getX() {return m_iX;}
    int getY() {return m_iY;}

    void setAnimation(THAnimationManager* pManager, unsigned int iAnimation);
    void setFrame(unsigned int iFrame);
    void setFlags(unsigned long iFlags) {this->iFlags = iFlags;}
    void setPosition(int iX, int iY) {m_iX = iX, m_iY = iY;}
    void setSpeed(int iX, int iY) {m_iSpeedX = iX, m_iSpeedY = iY;}
    void setLayer(int iLayer, int iId);

protected:
    THAnimationManager *m_pManager;
    unsigned int m_iAnimation;
    unsigned int m_iFrame;
    //! X position on tile (not tile x-index)
    int m_iX;
    //! Y position on tile (not tile y-index)
    int m_iY;
    //! Amount to change m_iX per tick
    int m_iSpeedX;
    //! Amount to change m_iY per tick
    int m_iSpeedY;
    THLayers_t m_oLayers;
};

#endif // CORSIX_TH_TH_GFX_H_