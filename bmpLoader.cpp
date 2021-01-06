#include <string>
#include <cinttypes>

static const char* bitmapFileMagic {0x4d4d};

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
// references: 
// [0] https://medium.com/sysf/bits-to-bitmaps-a-simple-walkthrough-of-bmp-image-format-765dc6857393)
// [1] https://devblogs.microsoft.com/oldnewthing/20200103-00/?p=103290

struct BitmapFileHeader
{
  static constexpr int size {14};

  uint16_t _fileMagic;
  uint32_t _fileSize;
  uint16_t _reserved0;
  uint16_t _reserved1;
  uint32_t _pixelOffset;
};

// note: There are multiple versions of the DIB header of BMP files identified by their header
// size. This module supports: BITMAPINFOHEADER, BITMAPV2INFOHEADER, BITMAPV3INFOHEADER,
// BITMAPV4HEADER, BITMAPV5HEADER. It does not support OS2 headers.
// references:
// [0] https://en.wikipedia.org/wiki/BMP_file_format

struct BitmapInfoHeader
{
  static constexpr int size_bytes {40};

  uint32_t _headerSize_bytes;
  uint32_t _bmpWidth_px;
  uint32_t _bmpHeight_px;
  uint16_t _numColorPlanes;
  uint16_t _bitsPerPixel; 
  uint32_t _compression; 
  uint32_t _rawImageSize_bytes;
  uint32_t _horizontalResolution_pxPm;
  uint32_t _verticalResolution_pxPm;
  uint32_t _numPaletteColors;
  uint32_t _numImportantColors;
};

struct BitmapV2InfoHeader
{
  static constexpr int size_bytes {52};

  uint32_t _headerSize_bytes;
  uint32_t _bmpWidth_px;
  uint32_t _bmpHeight_px;
  uint16_t _numColorPlanes;
  uint16_t _bitsPerPixel; 
  uint32_t _compression; 
  uint32_t _rawImageSize_bytes;
  uint32_t _horizontalResolution_pxPm;
  uint32_t _verticalResolution_pxPm;
  uint32_t _numPaletteColors;
  uint32_t _numImportantColors;
  uint32_t _redMask;
  uint32_t _greenMask;
  uint32_t _blueMask;
};

struct BitmapV3InfoHeader
{
  static constexpr int size_bytes {56};

  uint32_t _headerSize_bytes;
  uint32_t _bmpWidth_px;
  uint32_t _bmpHeight_px;
  uint16_t _numColorPlanes;
  uint16_t _bitsPerPixel; 
  uint32_t _compression; 
  uint32_t _rawImageSize_bytes;
  uint32_t _horizontalResolution_pxPm;
  uint32_t _verticalResolution_pxPm;
  uint32_t _numPaletteColors;
  uint32_t _numImportantColors;
  uint32_t _redMask;
  uint32_t _greenMask;
  uint32_t _blueMask;
  uint32_t _alphaMask;
};

struct BitmapV4Header
{
  static constexpr int size_bytes {108};

  uint32_t _headerSize_bytes;
  uint32_t _bmpWidth_px;
  uint32_t _bmpHeight_px;
  uint16_t _numColorPlanes;
  uint16_t _bitsPerPixel; 
  uint32_t _compression; 
  uint32_t _rawImageSize_bytes;
  uint32_t _horizontalResolution_pxPm;
  uint32_t _verticalResolution_pxPm;
  uint32_t _numPaletteColors;
  uint32_t _numImportantColors;
  uint32_t _redMask;
  uint32_t _greenMask;
  uint32_t _blueMask;
  uint32_t _alphaMask;
  uint32_t _colorSpaceMagic;
  uint32_t _unused[12];
};

struct BitmapV5Header
{
  static constexpr int size_bytes {124};

  uint32_t _headerSize_bytes;
  uint32_t _bmpWidth_px;
  uint32_t _bmpHeight_px;
  uint16_t _numColorPlanes;
  uint16_t _bitsPerPixel; 
  uint32_t _compression; 
  uint32_t _rawImageSize_bytes;
  uint32_t _horizontalResolution_pxPm;
  uint32_t _verticalResolution_pxPm;
  uint32_t _numPaletteColors;
  uint32_t _numImportantColors;
  uint32_t _redMask;
  uint32_t _greenMask;
  uint32_t _blueMask;
  uint32_t _alphaMask;
  uint32_t _colorSpaceMagic;
  uint32_t _unused[16];
};

// note: most of these compression formats are not supported by this module. Only BI_RGB (no
// compression) and BI_BITFIELDS (bit field masks) are supported. 
// references: 
// [0] https://en.wikipedia.org/wiki/BMP_file_format#Pixel_storage
enum BitmapFileCompressionMode
{
  BI_RGB, BI_RLE8, BI_RLE4, BI_BITFIELDS, BI_JPEG, BI_PNG, BI_ALPHABITFIELDS, BI_CMYK = 11,
  BI_CMYKRLE8, BI_CMYKRLE4
};

// The common header contains every member of all header versions. Each successive version
// adds members to the last thus the common header is the latest version.
using BitmapCommonInfoHeader = BitmapV5Header;

class BmpImage
{
public:
  int load(std::string filename);
private:
  void extractFileHeader(std::ifstream& file, BitmapFileHeader& header);
  void extractFileInfoHeaderSize(std::ifstream& file);
  void extractBitmapInfoHeader(std::ifstream& file, BitmapCommonInfoHeader& header);
  void extractBitmapV2InfoHeader(std::ifstream& file, BitmapCommonInfoHeader& header);
  void extractBitmapV3InfoHeader(std::ifstream& file, BitmapCommonInfoHeader& header);
  void extractBitmapV4Header(std::ifstream& file, BitmapCommonInfoHeader& header);
  void extractBitmapV5Header(std::ifstream& file, BitmapCommonInfoHeader& header);
  void extractColorPalette(std::ifstream& file, BitmapCommonInfoHeader& header);
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

  BitmapCommonInfoHeader infoHeader {};
  uint32_t infoHeaderSize = extractFileInfoHeaderSize(file);
  switch(infoHeaderSize)
  {
  case BitmapInfoHeader::size_bytes:
    extractBitmapInfoHeader(file, infoHeader);
    if(infoHeader._bitCount <= 8){
      extractColorPalette(file, infoHeader);
      extractPalettedPixels(file, infoHeader);
    }
    else if(infoHeader._bitCount == 16){
      if(infoHeader._compression == BI_RGB){
        uint16_t rmask {};
        uint16_t gmask {};
        uint16_t bmask {};
        uint16_t amask {};
        extractRGBA16Pixels(file, infoHeader, rmask, gmask, bmask, amask);
      }
      if(infoHeader._compression == BI_BITFIELDS){
        uint16_t rmask {};
        uint16_t gmask {};
        uint16_t bmask {};
        uint16_t amask {};

      }
    }
    else if(infoHeader._bitCount == 24){

    }
    else if(infoHeader._bitCount == 32){

    }
    break;
  case BitmapV2InfoHeader::size_bytes:
    extractBitmapV2InfoHeader(file, infoHeader);
    break;
  case BitmapV3InfoHeader::size_bytes:
    extractBitmapV3InfoHeader(file, infoHeader);
    break;
  case BitmapV4Header::size_bytes:
    extractBitmapV4Header(file, infoHeader);
    break;
  case BitmapV5Header::size_bytes:
    extractBitmapV5Header(file, infoHeader);
    break;
  default:
    return -1;
  };

  if(c && !(infoHeader._compression == BI_RGB || infoHeader._compression == BI_BITFIELDS)){
    return -1;
  }

  if(infoHeader._bitCount <= 8 && infoHeader._numPaletteColors == 0){

  }

  if((rgbm || am) && infoHeader._compression != BI_BITFIELDS){

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

uint32_t BmpImage::extractFileInfoHeaderSize(std::ifstream& file)
{
  uint8_t bytes[4];
  file.seekg(BitmapFileHeader::size_bytes);
  file.read(&bytes, 4);
  return extractLittleEndianUint32(bytes);
}

void BmpImage::extractBitmapInfoHeader(std::ifstream& file, BitmapCommonInfoHeader& header)
{
  uint8_t bytes[BitmapInfoHeader::size_bytes];
  file.seekg(BitmapFileHeader::size_bytes);
  file.read(&bytes, BitmapInfoHeader::size_bytes); 
  header._headerSize_bytes = extractLittleEndianUint32(bytes);
  header._bmpWidth_px = extractLittleEndianUint32(bytes + 4);
  header._bmpHeight_px = extractLittleEndianUint32(bytes + 8);
  header._numColorPlanes = extractLittleEndianUint16(bytes + 12);
  header._bitsPerPixel = extractLittleEndianUint16(bytes + 14);
  header._compression = extractLittleEndianUint32(bytes + 16);
  header._rawImageSize_bytes = extractLittleEndianUint32(bytes + 20);
  header._horizontalResolution_pxPm = extractLittleEndianUint32(bytes + 24);
  header._verticalResolution_pxPm = extractLittleEndianUint32(bytes + 28);
  header._numPaletteColors = extractLittleEndianUint32(bytes + 32);
  header._numImportantColors = extractLittleEndianUint32(bytes + 36);
}

void BmpImage::extractBitmapV2InfoHeader(std::ifstream& file, BitmapCommonInfoHeader& header)
{
  static constexpr int readSize_bytes = BitmapV2InfoHeader::size_bytes - BitmapInfoHeader::size_bytes;

  extractBitmapInfoHeader(file, header);
  uint8_t bytes[readSize_bytes];
  file.read(&bytes, readSize_bytes);
  header._redMask = extractLittleEndianUint32(bytes);
  header._greenMask = extractLittleEndianUint32(bytes + 4);
  header._blueMask = extractLittleEndianUint32(bytes + 4);
}

void BmpImage::extractBitmapV3InfoHeader(std::ifstream& file, BitmapCommonInfoHeader& header)
{
  static constexpr int readSize_bytes = BitmapV3InfoHeader::size_bytes - BitmapV2InfoHeader::size_bytes;

  extractBitmapV2InfoHeader(file, header);
  uint8_t bytes[readSize_bytes];
  file.read(&bytes, readSize_bytes);
  header._alphaMask = extractLittleEndianUint32(bytes);
}

void BmpImage::extractBitmapV4Header(std::ifstream& file, BitmapCommonInfoHeader& header)
{
  static constexpr int readSize_bytes = BitmapV4Header::size_bytes - BitmapV3InfoHeader::size_bytes;

  extractBitmapV3InfoHeader(file, header);
  uint8_t bytes[readSize_bytes];
  file.read(&bytes, readSize_bytes);
  header._colorSpaceMagic = extractLittleEndianUint32(bytes);
}

void BmpImage::extractBitmapV5Header(std::ifstream& file, BitmapCommonInfoHeader& header)
{
  extractBitmapV4Header(file, header);
}

