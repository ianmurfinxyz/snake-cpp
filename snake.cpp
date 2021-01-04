//----------------------------------------------------------------------------------------------//
//                  _                                                                           //
//                 | |                                                                          //
//  ___ _ __   __ _| | _____     __                                                             //
// / __| '_ \ / _` | |/ / _ \   {OO}                                                            //
// \__ \ | | | (_| |   <  __/   \__/                                                            //
// |___/_| |_|\__,_|_|\_\___|   |^|                                                             //
//  ____________________________/ /                                                             //
// /  ___________________________/                                                              //
//  \_______ \                                                                                  //
//          \|                                                                                  //
//                                                                                              //
// FILE: snake.cpp                                                                              //
// AUTHOR: Ian Murfin - github.com/ianmurfinxyz                                                 //
//                                                                                              //
// CREATED: 2nd Jan 2021                                                                        //
// UPDATED: 3rd Jan 2021                                                                        //
//                                                                                              //
//----------------------------------------------------------------------------------------------//

#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <sstream>
#include <cassert>
#include <cmath>
#include <array>
#include <vector>
#include <memory>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

namespace sk
{

//------------------------------------------------------------------------------------------------
//  MATH                                                                                          
//------------------------------------------------------------------------------------------------

struct Vector2i
{
  constexpr Vector2i() : _x{0}, _y{0} {}
  constexpr Vector2i(int32_t x, int32_t y) : _x{x}, _y{y} {}

  Vector2i(const Vector2i&) = default;
  Vector2i(Vector2i&&) = default;
  Vector2i& operator=(const Vector2i&) = default;
  Vector2i& operator=(Vector2i&&) = default;

  void zero() {_x = _y = 0;}
  bool isZero() {return _x == 0 && _y == 0;}
  Vector2i operator+(const Vector2i& v) const {return Vector2i{_x + v._x, _y + v._y};}
  void operator+=(const Vector2i& v) {_x += v._x; _y += v._y;}
  Vector2i operator-(const Vector2i& v) const {return Vector2i{_x - v._x, _y - v._y};}
  void operator-=(const Vector2i& v) {_x -= v._x; _y -= v._y;}
  Vector2i operator*(float scale) const {return Vector2i(_x * scale, _y * scale);}
  void operator*=(float scale) {_x *= scale; _y *= scale;}
  void operator*=(int32_t scale) {_x *= scale; _y *= scale;}
  float dot(const Vector2i& v) {return (_x * v._x) + (_y * v._y);}
  float cross(const Vector2i& v) const {return (_x * v._y) - (_y * v._x);}
  float length() const {return std::hypot(_x, _y);}
  float lengthSquared() const {return (_x * _x) + (_y * _y);}
  inline Vector2i normalized() const;
  inline void normalize();

  int32_t _x;
  int32_t _y;
};

Vector2i Vector2i::normalized() const
{
  Vector2i v = *this;
  v.normalize();
  return v;
}

void Vector2i::normalize()
{
  float l = (_x * _x) + (_y * _y);
  if(l) {
    l = std::sqrt(l);
    _x /= l;
    _y /= l;
  }
}

struct iRect
{
  int32_t _x;
  int32_t _y;
  int32_t _w;
  int32_t _h;
};

//------------------------------------------------------------------------------------------------
//  LOG                                                                                           
//------------------------------------------------------------------------------------------------

namespace logstr
{
  constexpr const char* fail_open_log = "failed to open log";
  constexpr const char* fail_sdl_init = "failed to initialize SDL";
  constexpr const char* fail_create_opengl_context = "failed to create opengl context";
  constexpr const char* fail_set_opengl_attribute = "failed to set opengl attribute";
  constexpr const char* fail_create_window = "failed to create window";

  constexpr const char* info_stderr_log = "logging to standard error";
  constexpr const char* info_creating_window = "creating window";
  constexpr const char* info_created_window = "window created";
  constexpr const char* using_opengl_version = "using opengl version";
}; 

class Log
{
public:
  enum Level { FATAL, ERROR, WARN, INFO };
public:
  Log();
  ~Log();
  Log(const Log&) = delete;
  Log(Log&&) = delete;
  Log& operator=(const Log&) = delete;
  Log& operator=(Log&&) = delete;
  void log(Level level, const char* error, const std::string& addendum = std::string{});
private:
  static constexpr const char* filename {"log"};
  static constexpr const char* delim {" : "};
  static constexpr std::array<const char*, 4> lvlstr {"fatal", "error", "warning", "info"};
private:
  std::ofstream _os;
};

Log::Log()
{
  _os.open(filename, std::ios_base::trunc);
  if(!_os){
    log(ERROR, logstr::fail_open_log);
    log(INFO, logstr::info_stderr_log);
  }
}

Log::~Log()
{
  if(_os)
    _os.close();
}

void Log::log(Level level, const char* error, const std::string& addendum)
{
  std::ostream& o {_os ? _os : std::cerr}; 
  o << lvlstr[level] << delim << error;
  if(!addendum.empty())
    o << delim << addendum;
  o << std::endl;
}

std::unique_ptr<Log> log {nullptr};

//------------------------------------------------------------------------------------------------
//  INPUT                                                                                       
//------------------------------------------------------------------------------------------------

class Input
{
public:
  enum KeyCode { 
    KEY_a, KEY_b, KEY_c, KEY_d, KEY_e, KEY_f, KEY_g, KEY_h, KEY_i, KEY_j, KEY_k, KEY_l, KEY_m, 
    KEY_n, KEY_o, KEY_p, KEY_q, KEY_r, KEY_s, KEY_t, KEY_u, KEY_v, KEY_w, KEY_x, KEY_y, KEY_z,
    KEY_SPACE, KEY_BACKSPACE, KEY_ENTER, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_COUNT 
  };
  struct KeyLog
  {
    bool _isDown;
    bool _isPressed;
    bool _isReleased;
  };
public:
  Input();
  ~Input() = default;
  void onKeyEvent(const SDL_Event& event);
  void onUpdate();
  bool isKeyDown(KeyCode key) {return _keys[key]._isDown;}
  bool isKeyPressed(KeyCode key) {return _keys[key]._isPressed;}
  bool isKeyReleased(KeyCode key) {return _keys[key]._isReleased;}
private:
  KeyCode convertSdlKeyCode(int sdlCode);
private:
  std::array<KeyLog, KEY_COUNT> _keys;
};

extern std::unique_ptr<Input> input;

Input::Input()
{
  for(auto& key : _keys)
    key._isDown = key._isReleased = key._isPressed = false;
}

void Input::onKeyEvent(const SDL_Event& event)
{
  assert(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP);

  KeyCode key = convertSdlKeyCode(event.key.keysym.sym);

  if(key == KEY_COUNT) 
    return;

  if(event.type == SDL_KEYDOWN){
    _keys[key]._isDown = true;
    _keys[key]._isPressed = true;
  }
  else{
    _keys[key]._isDown = false;
    _keys[key]._isReleased = true;
  }
}

void Input::onUpdate()
{
  for(auto& key : _keys)
    key._isPressed = key._isReleased = false;
}

Input::KeyCode Input::convertSdlKeyCode(int sdlCode)
{
  switch(sdlCode) {
    case SDLK_a: return KEY_a;
    case SDLK_b: return KEY_b;
    case SDLK_c: return KEY_c;
    case SDLK_d: return KEY_d;
    case SDLK_e: return KEY_e;
    case SDLK_f: return KEY_f;
    case SDLK_g: return KEY_g;
    case SDLK_h: return KEY_h;
    case SDLK_i: return KEY_i;
    case SDLK_j: return KEY_j;
    case SDLK_k: return KEY_k;
    case SDLK_l: return KEY_l;
    case SDLK_m: return KEY_m;
    case SDLK_n: return KEY_n;
    case SDLK_o: return KEY_o;
    case SDLK_p: return KEY_p;
    case SDLK_q: return KEY_q;
    case SDLK_r: return KEY_r;
    case SDLK_s: return KEY_s;
    case SDLK_t: return KEY_t;
    case SDLK_u: return KEY_u;
    case SDLK_v: return KEY_v;
    case SDLK_w: return KEY_w;
    case SDLK_x: return KEY_x;
    case SDLK_y: return KEY_y;
    case SDLK_z: return KEY_z;
    case SDLK_SPACE: return KEY_SPACE;
    case SDLK_BACKSPACE: return KEY_BACKSPACE;
    case SDLK_RETURN: return KEY_ENTER;
    case SDLK_LEFT: return KEY_LEFT;
    case SDLK_RIGHT: return KEY_RIGHT;
    case SDLK_DOWN: return KEY_DOWN;
    case SDLK_UP: return KEY_UP;
    default: return KEY_COUNT;
  }
}

std::unique_ptr<Input> input {nullptr};

//------------------------------------------------------------------------------------------------
//  RENDERER                                                                                      
//------------------------------------------------------------------------------------------------

class Color4
{
  constexpr static uint8_t i_lo {0};
  constexpr static uint8_t i_hi {255};
  constexpr static float f_lo {0.f};
  constexpr static float f_hi {1.f};

public:
  Color4() : _fr{0.f}, _fg{0.f}, _fb{0.f}, _fa{0.f}, _ir{0}, _ig{0}, _ib{0}, _ia{0}{}

  constexpr Color4(float r, float g, float b, float a = 0.f) : 
    _fr{std::clamp(r, f_lo, f_hi)},
    _fg{std::clamp(g, f_lo, f_hi)},
    _fb{std::clamp(b, f_lo, f_hi)},
    _fa{std::clamp(a, f_lo, f_hi)},
    _ir{static_cast<uint8_t>(_fr * 255)},
    _ig{static_cast<uint8_t>(_fg * 255)},
    _ib{static_cast<uint8_t>(_fb * 255)},
    _ia{static_cast<uint8_t>(_fa * 255)}
  {}

  constexpr Color4(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) :
    _ir{std::clamp(r, i_lo, i_hi)},
    _ig{std::clamp(g, i_lo, i_hi)},
    _ib{std::clamp(b, i_lo, i_hi)},
    _ia{std::clamp(a, i_lo, i_hi)},
    _fr{std::clamp(_ir / 255.f, f_lo, f_hi)},   // clamp to snip any floating point errors.
    _fg{std::clamp(_ir / 255.f, f_lo, f_hi)},
    _fb{std::clamp(_ir / 255.f, f_lo, f_hi)},
    _fa{std::clamp(_ir / 255.f, f_lo, f_hi)}
  {}

  Color4(const Color4&) = default;
  Color4(Color4&&) = default;
  Color4& operator=(const Color4&) = default;
  Color4& operator=(Color4&&) = default;

  float getfRed() const {return _fr;}
  float getfGreen() const {return _fg;}
  float getfBlue() const {return _fb;}
  float getfAlpha() const {return _fa;}
  uint8_t getiRed() const {return _ir;}
  uint8_t getiGreen() const {return _ig;}
  uint8_t getiBlue() const {return _ib;}
  uint8_t getiAlpha() const {return _ia;}
  void setRed(float r){_fr = std::clamp(r, f_lo, f_hi);}
  void setGreen(float g){_fg = std::clamp(g, f_lo, f_hi);}
  void setBlue(float b){_fb = std::clamp(b, f_lo, f_hi);}

private:
  float _fr;
  float _fg;
  float _fb;
  float _fa;
  uint8_t _ir;
  uint8_t _ig;
  uint8_t _ib;
  uint8_t _ia;
};

namespace colors
{
constexpr Color4 white {1.f, 1.f, 1.f};
constexpr Color4 black {0.f, 0.f, 0.f};
constexpr Color4 red {1.f, 0.f, 0.f};
constexpr Color4 green {0.f, 1.f, 0.f};
constexpr Color4 blue {0.f, 0.f, 1.f};
constexpr Color4 cyan {0.f, 1.f, 1.f};
constexpr Color4 magenta {1.f, 0.f, 1.f};
constexpr Color4 yellow {1.f, 1.f, 0.f};

// greys - more grays: https://en.wikipedia.org/wiki/Shades_of_gray 

constexpr Color4 gainsboro {0.88f, 0.88f, 0.88f};
constexpr Color4 lightgray {0.844f, 0.844f, 0.844f};
constexpr Color4 silver {0.768f, 0.768f, 0.768f};
constexpr Color4 mediumgray {0.76f, 0.76f, 0.76f};
constexpr Color4 spanishgray {0.608f, 0.608f, 0.608f};
constexpr Color4 gray {0.512f, 0.512f, 0.512f};
constexpr Color4 dimgray {0.42f, 0.42f, 0.42f};
constexpr Color4 davysgray {0.34f, 0.34f, 0.34f};
constexpr Color4 jet {0.208f, 0.208f, 0.208f};
};

class Renderer
{
public:
  struct Config
  {
    std::string _windowTitle;
    int32_t _windowWidth;
    int32_t _windowHeight;
  };
public:
  Renderer(const Config& config);
  Renderer(const Renderer&) = delete;
  Renderer* operator=(const Renderer&) = delete;
  ~Renderer();
  void setViewport(iRect viewport);
  void clearWindow(const Color4& color);
  void clearViewport(const Color4& color);
  void drawBlockArray(int first, int count, void* blockData, int blockPixelSize);
  void show();
  void getWindowSize(int& w, int& h) const;
private:
  static constexpr int openglVersionMajor = 2;
  static constexpr int openglVersionMinor = 1;
private:
  SDL_Window* _window;
  SDL_GLContext _glContext;
  Config _config;
  iRect _viewport;
};

Renderer::Renderer(const Config& config)
{
  _config = config;

  std::stringstream ss {};
  ss << "{w:" << _config._windowWidth << ",h:" << _config._windowHeight << "}";
  sk::log->log(Log::INFO, logstr::info_creating_window, std::string{ss.str()});

  _window = SDL_CreateWindow(
      _config._windowTitle.c_str(), 
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      _config._windowWidth,
      _config._windowHeight,
      SDL_WINDOW_OPENGL
  );

  if(_window == nullptr){
    sk::log->log(Log::FATAL, logstr::fail_create_window, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  int w, h;
  getWindowSize(w, h);
  std::stringstream().swap(ss);
  ss << "{w:" << w << ",h:" << h << "}";
  sk::log->log(Log::INFO, logstr::info_created_window, std::string{ss.str()});

  _glContext = SDL_GL_CreateContext(_window);
  if(_glContext == nullptr){
    sk::log->log(Log::FATAL, logstr::fail_create_opengl_context, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, openglVersionMajor) < 0){
    sk::log->log(Log::FATAL, logstr::fail_set_opengl_attribute, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, openglVersionMinor) < 0){
    sk::log->log(Log::FATAL, logstr::fail_set_opengl_attribute, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  sk::log->log(Log::INFO, logstr::using_opengl_version, std::string{reinterpret_cast<const char*>(glGetString(GL_VERSION))});

  setViewport(iRect{0, 0, _config._windowWidth, _config._windowHeight});
}

Renderer::~Renderer()
{
  SDL_GL_DeleteContext(_glContext);
  SDL_DestroyWindow(_window);
}

void Renderer::setViewport(iRect viewport)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, viewport._w, 0.0, viewport._h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(viewport._x, viewport._y, viewport._w, viewport._h);
  _viewport = viewport;
}

void Renderer::clearWindow(const Color4& color)
{
  glClearColor(color.getfRed(), color.getfGreen(), color.getfBlue(), color.getfAlpha());
  glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::clearViewport(const Color4& color)
{
  glEnable(GL_SCISSOR_TEST);
  glScissor(_viewport._x, _viewport._y, _viewport._w, _viewport._h);
  glClearColor(color.getfRed(), color.getfGreen(), color.getfBlue(), color.getfAlpha());
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

void Renderer::drawBlockArray(int first, int count, void* blockData, int blockPixelSize)
{
  glInterleavedArrays(GL_C4UB_V2F, 0, blockData);
  glPointSize(blockPixelSize - 2);
  glDrawArrays(GL_POINTS, first, count);
}

void Renderer::show()
{
  SDL_GL_SwapWindow(_window);
}

void Renderer::getWindowSize(int& w, int& h) const
{
  SDL_GL_GetDrawableSize(_window, &w, &h);
}

std::unique_ptr<Renderer> renderer {nullptr};

//------------------------------------------------------------------------------------------------
//  SNAKE                                                                                         
//------------------------------------------------------------------------------------------------

//class Palette
//{
//public:
//  static constexpr int numColors = 8;
//public:
//  Palette() = default; 
//  ~Palette() = default;
//  void setColor(const Color3f& color, int index);
//  const Color3f& getColor(int index) const;
//private:
//  std::array<Color3f, numColors> _colors;
//};
//
//void Palette::setColor(const Color3f& color, int index)
//{
//  assert(0 <= index && index < numColors);
//  _colors[index] = color;
//}
//
//const Color3f& Palette::getColor(int index) const
//{
//  assert(0 <= index && index < numColors);
//  return _colors[index];
//}

// A 2D world of blocks. Each block in the world is simply an integer index into a color
// palette. By default all unset blocks have a value of 0 which is thus color 0 into the
// palette. Hence color 0 is used as the background color of the world.
//
// The position of the block world is taken as the bottom-left corner and the grid is laid
// out as shown below:
//
//      row
//       ^
//       |
//       |
//   pos o----> col
//
//class BlockWorld
//{
//  using BlockRow = std::vector<uint8_t>;
//  using BlockGrid = std::vector<BlockRow>;
//public:
//  BlockWorld(Vector2i position, Vector2i dimensions, const Palette& palette);
//  ~BlockWorld() = default;
//  void draw();
//  void setBlock(int row, int col, int colorIndex);
//  const Color3f& getBlock(int row, int col) const;
//private:
//  static constexpr int blockSize_px = 8; 
//  static std::array<uint8_t, 8> bitmap;   // must be non-const to be passed to glBitmap.
//private:
//  void populate();
//  void clear();
//private:
//  BlockGrid _grid;
//  Vector2i _dimensions; // [x:width, y:height] in blocks.
//  Vector2i _position;   // w.r.t screen space.
//  const Palette& _palette;
//};
//
//std::array<uint8_t, 8> BlockWorld::bitmap {0x00, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x00};
//
//BlockWorld::BlockWorld(Vector2i position, Vector2i dimensions, const Palette& palette) :
//  _grid{},
//  _dimensions{dimensions},
//  _position{position},
//  _palette{palette}
//{
//  populate();
//}
//
//void BlockWorld::draw()
//{
//  // TODO - This function is really slow!!! On my system it is taking ~47ms with a world
//  // of 50x50 blocks! Find the bottleneck and a more efficient rendering method.
//  //
//  // I never had an issue with this with the space invaders game since the number of glbitmap
//  // calls is considerably lower (on the order of a few hundred at most) whereas here I am working
//  // with ~2500+ calls. According to this discussion:
//  //          https://stackoverflow.com/questions/3339877/glbitmap-questions
//  // I should just go with textures quads. Which I can do no prob so should make the switch. I 
//  // actually wont need to texture them, can simply draw colored quads.
//  
//  int64_t sum {0};
//
//  int y {_position._y};
//  int colorIndex {-1};
//  sk::renderer->setRasterPos(_position._x, y);
//  for(auto& row : _grid){
//    for(auto& block : row){
//      if(colorIndex != block){
//        sk::renderer->setDrawColor(_palette.getColor(block)); 
//        colorIndex = block;
//      }
//
//      auto now0 = std::chrono::high_resolution_clock::now();
//      sk::renderer->blitBitmap(blockSize_px, blockSize_px, 0, 0, blockSize_px, 0, bitmap.data()); 
//      auto now1 = std::chrono::high_resolution_clock::now();
//      sum += std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0).count();
//    }
//    y += blockSize_px;
//    sk::renderer->setRasterPos(_position._x, y);
//  }
//
//  sum /= 50 * 50;
//
//  std::cout << "blitBitmap execution avg time (us): " 
//            << sum
//            << std::endl;
//
//}
//
//void BlockWorld::populate()
//{
//  _grid.reserve(_dimensions._y);
//  for(int row = 0; row < _dimensions._y; ++row){
//    BlockRow blockRow{};
//    blockRow.reserve(_dimensions._x);
//    for(int col = 0; col < _dimensions._x; ++col)
//      blockRow.push_back(0);
//    _grid.push_back(std::move(blockRow));
//  }
//}
//
//void BlockWorld::clear()
//{
//  for(auto& row : _grid)
//    for(auto& block : row)
//      block = 0;
//}

class BlockWorld
{
public:
  BlockWorld(Vector2i position);
  ~BlockWorld() = default;
  void setBlock(int row, int col, const Color4& color);
  void clear(const Color4& color);
  void clear(iRect region, const Color4& color);
  void draw();
private:
  // 12 byte blocks designed to work with glInterleavedArrays format GL_C4UB_V2F.
  struct Block
  {
    uint8_t _red;
    uint8_t _green;
    uint8_t _blue;
    uint8_t _alpha;
    float _x;
    float _y;
  };
private:
  static constexpr int blockPixelSize = 11;  // use odd number so center of block is actually central.
  static constexpr int worldBlockWidth = 50;
  static constexpr int worldBlockHeight = 50;
  static constexpr int worldBlockCount = worldBlockWidth * worldBlockHeight;
private:
  Vector2i _position;
  std::array<Block, worldBlockCount> _blocks; // flattened 2D array accessed (col + (row * width))
};

BlockWorld::BlockWorld(Vector2i position) : 
  _position{position}
{
  int blockCenterOffset = (blockPixelSize + 1) / 2;
  for(int col = 0; col < worldBlockWidth; ++col){
    for(int row = 0; row < worldBlockWidth; ++row){
      int index = col + (row * worldBlockWidth);
      Block& block = _blocks[index];
      block._red = 0;
      block._green = 0;
      block._blue = 0;
      block._alpha = 0;
      block._x = _position._x + (col * blockPixelSize) + blockCenterOffset;
      block._y = _position._y + (row * blockPixelSize) + blockCenterOffset;
    }
  }
}

void BlockWorld::setBlock(int row, int col, const Color4& color)
{
  assert(0 <= row && row < worldBlockHeight);
  assert(0 <= col && col < worldBlockWidth);
  int index = col + (row * worldBlockWidth);
  Block& block = _blocks[index];
  block._red = color.getiRed();
  block._green = color.getiGreen();
  block._blue = color.getiBlue();
  block._alpha = color.getiAlpha();
}

void BlockWorld::clear(const Color4& color)
{
  uint8_t r, g, b, a;
  r = color.getiRed();
  g = color.getiGreen();
  b = color.getiBlue();
  a = color.getiAlpha();
  for(int col = 0; col < worldBlockWidth; ++col){
    for(int row = 0; row < worldBlockWidth; ++row){
      int index = col + (row * worldBlockWidth);
      Block& block = _blocks[index];
      block._red = r;
      block._green = g;
      block._blue = b;
      block._alpha = a;
    }
  }
}

void BlockWorld::clear(iRect region, const Color4& color)
{

}

void BlockWorld::draw()
{
  auto now0 = std::chrono::high_resolution_clock::now();
  sk::renderer->drawBlockArray(0, worldBlockCount, static_cast<void*>(_blocks.data()), blockPixelSize);
  auto now1 = std::chrono::high_resolution_clock::now();
  std::cout << "BlockWorld::draw execution time (us): "
            << std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0).count()
            << std::endl;
}

class Snake
{
public:
  Snake();
  ~Snake() = default;
  void draw();
private:
  BlockWorld _world;
};

Snake::Snake() : 
  _world(Vector2i{10,10})
{
  _world.clear(colors::gainsboro);
}

void Snake::draw()
{
  _world.draw();
}

//------------------------------------------------------------------------------------------------
//  APP                                                                                           
//------------------------------------------------------------------------------------------------

class App
{
public:
  using Clock_t = std::chrono::steady_clock;
  using TimePoint_t = std::chrono::time_point<Clock_t>;
  using Duration_t = std::chrono::nanoseconds;
private:
  class RealClock
  {
  public:
    RealClock() : _start{}, _now0{}, _now1{}, _dt{}{}
    ~RealClock() = default;
    void start() {_now0 = _start = Clock_t::now();}
    Duration_t update();
    Duration_t getDt() const {return _dt;}
    Duration_t getNow() const {return _now1 - _start;}
  private:
    TimePoint_t _start;
    TimePoint_t _now0;
    TimePoint_t _now1;
    Duration_t _dt;
  };
  class Metronome
  {
  public:
    Metronome(Duration_t appNow, Duration_t tickPeriod_ns);
    ~Metronome() = default;
    int64_t doTicks(Duration_t appNow);
    Duration_t getTickPeriod_ns() const {return _tickPeriod_ns;}
    float getTickPeriod_s() const {return _tickPeriod_s;}
  private:
    Duration_t _lastTickNow;
    Duration_t _tickPeriod_ns;
    float _tickPeriod_s;
    int64_t _totalTicks;
  };
public:
  App();
  ~App();
  App(const App&) = delete;
  App(const App&&) = delete;
  App& operator=(const App&) = delete;
  App& operator=(App&&) = delete;
  void initialize();
  void shutdown();
  void run();
private:
  void loop();
  void onTick(float dt);
private:
  static constexpr const char* name = "snake";
  static constexpr int appVersionMajor = 0;
  static constexpr int appVersionMinor = 1;
  static constexpr int windowWidth_px = 600;
  static constexpr int windowHeight_px = 600;
  static constexpr int maxTicksPerFrame = 5;
  static constexpr Duration_t minFramePeriod {static_cast<int64_t>(0.01e9)};
private:
  RealClock _clock;
  Metronome _metronome;
  int64_t _ticksAccumulated;
  bool _isDone;

  Snake _game;
};

App::Duration_t App::RealClock::update()
{
  _now1 = Clock_t::now();
  _dt = _now1 - _now0;
  _now0 = _now1;
  return _dt;
}

App::Metronome::Metronome(App::Duration_t appNow, App::Duration_t tickPeriod_ns) :
  _lastTickNow{appNow},
  _tickPeriod_ns{tickPeriod_ns},
  _tickPeriod_s{static_cast<float>(_tickPeriod_ns.count()) / 1.0e9f},
  _totalTicks{0}
{
}

int64_t App::Metronome::doTicks(Duration_t appNow)
{
  int64_t ticks;
  while(_lastTickNow + _tickPeriod_ns < appNow){
    _lastTickNow += _tickPeriod_ns;
    ++ticks;
  }
  _totalTicks += ticks;
  return ticks;
}

App::App() : 
  _clock{}, 
  _metronome{_clock.getNow(), Duration_t{static_cast<int64_t>(0.016e9)}},
  _ticksAccumulated{0},
  _isDone{false}
{
}

App::~App()
{
}

void App::initialize()
{
  sk::log = std::make_unique<Log>();
  sk::input = std::make_unique<Input>();

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    sk::log->log(Log::FATAL, logstr::fail_sdl_init, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  std::stringstream ss{};
  ss << name
     << " - version: "
     << appVersionMajor
     << "."
     << appVersionMinor;

  Renderer::Config rconfig {std::string{ss.str()}, windowWidth_px, windowHeight_px};
  renderer = std::make_unique<Renderer>(rconfig);
}

void App::shutdown()
{
  sk::log.reset(nullptr);
  sk::input.reset(nullptr);
  sk::renderer.reset(nullptr);
}

void App::run()
{
  while(!_isDone)
    loop();
}

void App::loop()
{
  auto now0 = Clock_t::now();
  auto realDt = _clock.update();
  auto realNow = _clock.getNow();

  SDL_Event event;
  while(SDL_PollEvent(&event) != 0){
    switch(event.type){
      case SDL_QUIT:
        _isDone = true;
        return;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        sk::input->onKeyEvent(event);
    }
  }

  _ticksAccumulated += _metronome.doTicks(realNow);
  int64_t ticksDoneThisFrame {0};
  while(_ticksAccumulated > 0 && ticksDoneThisFrame < maxTicksPerFrame){
    ++ticksDoneThisFrame;
    --_ticksAccumulated;
    onTick(_metronome.getTickPeriod_s());
  }

  sk::input->onUpdate();

  auto now1 = Clock_t::now();
  auto framePeriod = now1 - now0;
  if(framePeriod < minFramePeriod)
    std::this_thread::sleep_for(minFramePeriod - framePeriod);
}

void App::onTick(float dt)
{
  sk::renderer->clearWindow(colors::jet);
  _game.draw();
  sk::renderer->show();
}

std::unique_ptr<App> app {nullptr};

}; // namespace sk

//------------------------------------------------------------------------------------------------
//  MAIN                                                                                          
//------------------------------------------------------------------------------------------------

int main()
{
  sk::app = std::make_unique<sk::App>();
  sk::app->initialize();
  sk::app->run();
  sk::app->shutdown();
  sk::app.reset(nullptr);
}

