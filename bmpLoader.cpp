#include <string>
#include <cstring>
#include <cinttypes>

static const char* bitmapFileMagic {0x4d4d};
static const char* colorSpaceSRGBMagic {0x73524742};
void reverseBytes(uint8* bytes, int count)
{
  int swaps = ((count % 2) == 1) : (count - 1) / 2 : count / 2;
  uint8_t tmp;
  for(int i = 0; i < swaps; ++i){
    tmp = bytes[i]; 
    bytes[i] = bytes[count - 1 - i];
    bytes[count - i] = tmp;
  }
}

// reminder: Endianess determines the order in which bytes are stored in memory. Consider a 
// 32-bit integer 'n' assigned the hex value 0xa3b2c1d0. Its memory layout on each system can 
// be illustrated as:
//
//    lower addresses --------------------------------------> higher addresses
//            +----+----+----+----+            +----+----+----+----+
//            |0xd0|0xc1|0xb2|0xa3|            |0xa3|0xb2|0xc1|0xd0|
//            +----+----+----+----+            +----+----+----+----+
//            |                                |
//            &x                               &x
//
//              [little-endian]                      [big-endian]
//
//         little-end (LSB) of x at            big-end (MSB) of x at
//         lower address.                      lower address.
//
// Independent of the endianess however &x always returns the byte at the lower address due to
// the way C/C++ implements pointer arithmetic. If this were not the case you could not reliably
// increment a pointer to move through an array or buffer, e.g,
//
//   char buffer[10];
//   char* p = buffer;
//   for(int i{0}; i<10; ++i)
//     cout << (*p)++ << endl;
//
// would be platform dependent if '&' (address-of) operator returned the address of the LSB and
// not (as it does) the lowest address used by the operator target. If the former were true you
// would have to increment the pointer on little-endian systems and decrement it on big-endian
// systems.
//
// Thus this function takes advantage of the platform independence of the address-of operator to
// test for endianess.
bool isSystemLittleEndian()
{
  uint32_t n {0x00000001};                           // LSB == 1.
  uint8_t* p = reinterpret_cast<uint8_t*>(&n);       // pointer to byte at lowest address.
  return (*p);                                       // value of byte at lowest address.
}

// Extracts a type T from a byte buffer containing the bytes of T stored in little-endian 
// format (in the buffer). The endianess of the system is accounted for.
template<typename T>
T extractLittleEndian(uint8_t* buffer)
// predicate: buffer size is at least sizeof(T) bytes.
{
  if(!isSystemLittleEndian())
    reverseBytes(buffer, 2);
  return *(reinterpret_cast<T*>(buffer));
}

using extractLittleEndianUint16 = extractLittleEndian<uint16_t>;
using extractLittleEndianUint32 = extractLittleEndian<uint32_t>;
using extractLittleEndianUint64 = extractLittleEndian<uint64_t>;
using extractLittleEndianInt16 = extractLittleEndian<int16_t>;
using extractLittleEndianInt32 = extractLittleEndian<int32_t>;
using extractLittleEndianInt64 = extractLittleEndian<int64_t>;

// note: I am choosing NOT to pack these structs for use with reading binary data from a stream;
// struct packing can lead to problems on certain platforms. Read binary data into arrays and
// extract the data manually.

struct BitmapFileHeader
{
  static constexpr int size {14};

  uint16_t _fileMagic;
  uint32_t _fileSize;
  uint16_t _reserved0;
  uint16_t _reserved1;
  uint32_t _pixelOffset;
};

// There are multiple versions of the info (DIB) header of BMP files identified by their 
// header size. This module supports: BITMAPCOREHEADER, BITMAPINFOHEADER, BITMAPV2INFOHEADER, 
// BITMAPV3INFOHEADER, BITMAPV4HEADER, BITMAPV5HEADER. It does not support OS2 headers.
//
// note: each version extends the last rather than revising it thus all versions have been
// combined here into a single version (effectively the latest).
//
// note: the version BITMAPINFOHEADER is apparantly the commonly used version by software
// seeking to maintain backwards compatibility.
//
// For more information on the bitmap file format see references.
//
// references:
// [0] https://en.wikipedia.org/wiki/BMP_file_format
// [1] https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
// [2] https://medium.com/sysf/bits-to-bitmaps-a-simple-walkthrough-of-bmp-image-format-765dc6857393

struct BitmapInfoHeader
{
  // -- BITMAPCOREHEADER --
  
  static constexpr int BITMAPCOREHEADER_SIZE_BYTES {12};
  
  uint32_t _headerSize_bytes;
  uint32_t _bmpWidth_px;
  uint32_t _bmpHeight_px;
  uint16_t _numColorPlanes;
  uint16_t _bitsPerPixel; 

  // -- added BITMAPINFOHEADER --

  static constexpr int BITMAPINFOHEADER_SIZE_BYTES {40};

  uint32_t _compression; 
  uint32_t _rawImageSize_bytes;
  uint32_t _horizontalResolution_pxPm;
  uint32_t _verticalResolution_pxPm;
  uint32_t _numPaletteColors;
  uint32_t _numImportantColors;

  // -- added BITMAPV2INFOHEADER --

  static constexpr int BITMAPV2INFOHEADER_SIZE_BYTES {52};
  
  uint32_t _redMask;
  uint32_t _greenMask;
  uint32_t _blueMask;

  // -- added BITMAPV3INFOHEADER --
  
  static constexpr int BITMAPV3INFOHEADER_SIZE_BYTES {56};

  uint32_t _alphaMask;

  // -- added BITMAPV4HEADER --

  static constexpr int BITMAPV4HEADER_SIZE_BYTES {108};

  uint32_t _colorSpaceMagic;
  uint32_t _unused0[12];

  // -- added BITMAPV5HEADER --
  
  static constexpr int BITMAPV5HEADER_SIZE_BYTES {124};

  uint32_t _unused1[4];
};

enum BitmapInfoHeaderOffsets
{
  BIHO_HEADER_SIZE = 0,
  BIHO_BMP_WIDTH = 4,
  BIHO_BMP_HEIGHT = 8,
  BIHO_NUM_COLOR_PLANES = 12,
  BIHO_BITS_PER_PIXEL = 14,
  BIHO_COMPRESSION = 16,
  BIHO_RAW_IMAGE_SIZE = 20,
  BIHO_HORIZONTAL_RESOLUTION = 24,
  BIHO_VERTICAL_RESOLUTION = 28,
  BIHO_NUM_PALETTE_COLORS = 32,
  BIHO_NUM_IMPORTANT_COLORS = 36,
  BIHO_RED_MASK = 40,
  BIHO_GREEN_MASK = 44,
  BIHO_BLUE_MASK = 48,
  BIHO_ALPHA_MASK = 52,
  BIHO_COLOR_SPACE_MAGIC = 56
};

// note: most of these compression formats are not supported by this loader. Only BI_RGB (no
// compression) and BI_BITFIELDS (bit field masks) are supported, RLE (run-length-encoding)
// modes are not supported.
enum BitmapFileCompressionMode
{
  BI_RGB, BI_RLE8, BI_RLE4, BI_BITFIELDS, BI_JPEG, BI_PNG, BI_ALPHABITFIELDS, BI_CMYK = 11,
  BI_CMYKRLE8, BI_CMYKRLE4
};

class BmpImage
{
public:
  int load(std::string filename);
private:
  void extractFileHeader(std::ifstream& file, BitmapFileHeader& header);
  void extractInfoHeader(std::ifstream& file, BitmapInfoHeader& header);
  void extractAppendedRGBMasks(std::ifstream& file, BitmapInfoHeader& header);
  void extractColorPalette(std::ifstream& file, BitmapInfoHeader& header);
  void extractPalettedPixels();
  void extractPixels(std::ifstream& file, BitmapInfoHeader* header);
private:
  std::vector<Color4> _pallete;
};

int BmpImage::load(std::string filename)
{
  std::ifstream file {filename, std::ios_base::binary};
  if(!file){
    return -1;
  }

  BitmapFileHeader fileHeader {};
  extractFileHeader(file, fileHeader);
  if(fileHeader._fileMagic != bitmapFileMagic){
    return -1;
  }

  BitmapInfoHeader infoHeader {};
  memset(&infoHeader, 0, sizeof(infoHeader));
  extractInfoHeader(file, infoHeader);

  // other compression modes are not supported.
  if(infoHeader._compression != BI_RGB || infoHeader._compression != BI_BITFIELDS){
    return -1;
  }

  bool V2 {false}, V3_4_5 {false};
  switch(infoHeader._headerSize)
  {
  case BitmapInfoHeader::BITMAPV5HEADER_SIZE_BYTES:

    // fallthrough
    
  case BitmapInfoHeader::BITMAPV4HEADER_SIZE_BYTES:
    // other color spaces not supported.
    if(infoHeader._colorSpaceMagic != colorSpaceSRGBMagic){
      return -1;
    }

    // fallthrough
    
  case BitmapInfoHeader::BITMAPV3INFOHEADER_SIZE_BYTES:
    V3_4_5 = true;

    // fallthrough
    

  case BitmapInfoHeader::BITMAPV2INFOHEADER_SIZE_BYTES:
    if(!V3_4_5)
      V2 = true;

    // fallthrough
    
  case BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES:
    if(infoHeader._bitCount <= 8){
      extractColorPalette(file, infoHeader);
      extractPalettedPixels(file, infoHeader);
    }
    else if(infoHeader._bitCount == 16){
      if(infoHeader._compression == BI_RGB){
        // default masks for this scenario.
        infoHeader._redMask   = 0b00000000000000000111110000000000;
        infoHeader._greenMask = 0b00000000000000000000001111100000;
        infoHeader._blueMask  = 0b00000000000000000000000000011111;

        // BITMAPV3INFOHEADER uses set defaults for RGB masks but a custom alpha mask, thus we
        // dont want to override this custom mask if V3 info header is present.
        if(!V3_4_5)
          infoHeader._alphaMask = 0b00000000000000001000000000000000;

        extractPixels(file, infoHeader); 
      }
      if(infoHeader._compression == BI_BITFIELDS){
        // with BI_BITFIELDS and BITMAPINFOHEADER the masks are found appended to the end of 
        // the info header in an extra block of 12 bytes rather than in the header itself.
        if(!V2 && !V3_4_5)
          extractAppendedRGBMasks(file, infoHeader);

        extractPixels(file, infoHeader); 
      }
    }
    else if(infoHeader._bitCount == 24){
      // default masks for this scenario.
      infoHeader._redMask   = 0b00000000111111110000000000000000;
      infoHeader._greenMask = 0b00000000000000001111111100000000;
      infoHeader._blueMask  = 0b00000000000000000000000011111111;
      infoHeader._alphaMask = 0b00000000000000000000000000000000;
      extractPixels(file, infoHeader); 
    }
    else if(infoHeader._bitCount == 32){
      if(infoHeader._compression == BI_RGB){
        // default masks for this scenario.
        infoHeader._redMask   = 0b00000000111111110000000000000000;
        infoHeader._greenMask = 0b00000000000000001111111100000000;
        infoHeader._blueMask  = 0b00000000000000000000000011111111;

        // BITMAPV3INFOHEADER uses set defaults for RGB masks but a custom alpha mask, thus we
        // dont want to override this custom mask if V3 info header is present.
        if(!V3_4_5)
          infoHeader._alphaMask = 0b11111111000000000000000000000000;

        extractPixels(file, infoHeader); 
      }
      else if(info_header._compression == BI_BITFIELDS){
        // with BI_BITFIELDS and BITMAPINFOHEADER the masks are found appended to the end of 
        // the info header in an extra block of 12 bytes rather than in the header itself.
        if(!V2 && !V3_4_5)
          extractAppendedRGBMasks(file, infoHeader);

        extractPixels(file, infoHeader); 
      }
    }
  }
}

void BmpImage::extractFileHeader(std::ifstream& file, BitmapFileHeader& header)
{
  uint8_t bytes[BitmapFileHeader::size_bytes];
  file.seekg(0);
  file.read(&bytes, BitmapFileHeader::size_bytes);
  header._fileMagic = extractLittleEndianUint16(bytes);
  header._fileSize = extractLittleEndianUint32(bytes + 2);
  header._pixelOffset = extractLittleEndianUint32(bytes + 10);
}

void BmpImage::extractInfoHeader(std::ifstream& file, BitmapCommonInfoHeader& header)
{
  uint8_t bytes[BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES];

  // start by reading the header size to determine the info header version present.
  file.seekg(BitmapFileHeader::size_bytes);
  file.read(&bytes, sizeof(BitmapFileHeader._headerSize)); 
  header._headerSize = extractLittleEndianUint32(bytes);

  int seekPos, readSize;
  switch(header._headerSize)
  {
  case BitmapInfoHeader::BITMAPV5HEADER_SIZE_BYTES:
  case BitmapInfoHeader::BITMAPV4HEADER_SIZE_BYTES:
    seekPos = BitmapFileHeader::size_bytes + BIHO_COLOR_SPACE_MAGIC;
    readSize = sizeof(BitmapInfoHeader::_colorSpaceMagic);
    file.seekg(seekPos);
    file.read(bytes, readSize);
    header._colorSpaceMagic = extractLittleEndianUint32(bytes);

    // fallthrough

  case BitmapInfoHeader::BITMAPV3INFOHEADER_SIZE_BYTES:
    seekPos = BitmapFileHeader::size_bytes + BIHO_ALPHA_MASK;
    readSize = sizeof(BitmapInfoHeader::_alphaMask);
    file.seekg(seekPos);
    file.read(bytes, readSize);
    header._alphaMask = extractLittleEndianUint32(bytes);

    // fallthrough
    
  case BitmapInfoHeader::BITMAPV2INFOHEADER_SIZE_BYTES:
    seekPos = BitmapFileHeader::size_bytes + BIHO_RED_MASK;
    readSize = BIHO_ALPHA_MASK - BIHO_RED_MASK;
    file.seekg(seekPos);
    file.read(bytes, readSize);
    header._redMask = extractLittleEndianUint32(bytes);
    header._greenMask = extractLittleEndianUint32(bytes + 4);
    header._blueMask = extractLittleEndianUint32(bytes + 8);

    // fallthrough
    
  case BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES:
    seekPos = BitmapFileHeader::size_bytes;
    readSize = BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES;
    file.seekg(seekPos);
    file.read(bytes, readSize);
    header._bmpWidth_px = extractLittleEndianUint32(bytes + BIHO_BMP_WIDTH);
    header._bmpHeight_px = extractLittleEndianUint32(bytes + BIHO_BMP_HEIGHT);
    header._numColorPlanes = extractLittleEndianUint16(bytes + BIHO_NUM_COLOR_PLANES);
    header._bitsPerPixel = extractLittleEndianUint16(bytes + BIHO_BITS_PER_PIXEL);
    header._compression = extractLittleEndianUint32(bytes + BIHO_COMPRESSION);
    header._rawImageSize_bytes = extractLittleEndianUint32(bytes + BIHO_RAW_IMAGE_SIZE);
    header._horizontalResolution_pxPm = extractLittleEndianUint32(bytes + BIHO_HORIZONTAL_RESOLUTION);
    header._verticalResolution_pxPm = extractLittleEndianUint32(bytes + BIHO_VERTICAL_RESOLUTION);
    header._numPaletteColors = extractLittleEndianUint32(bytes + BIHO_NUM_PALETTE_COLORS);
    header._numImportantColors = extractLittleEndianUint32(bytes + BIHO_NUM_IMPORTANT_COLORS);
  }
}

void BmpImage::extractAppendedRGBMasks(std::ifstream& file, BitmapInfoHeader& header)
{

}

void BmpImage::extractColorPalette(std::ifstream& file, BitmapInfoHeader& header)
{

}

void BmpImage::extractPalettedPixels()
{

}

void BmpImage::extractPixels(std::ifstream& file, BitmapInfoHeader* header)
{

}

