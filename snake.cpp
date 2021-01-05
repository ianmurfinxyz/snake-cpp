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
// UPDATED: 4th Jan 2021                                                                        //
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
//  GFX                                                                                           
//------------------------------------------------------------------------------------------------

class Color4
{
private:
  constexpr static float f_lo {0.f};
  constexpr static float f_hi {1.f};

public:
  Color4() : _r{0}, _g{0}, _b{0}, _a{0}{}

  constexpr Color4(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) :
    _r{r},
    _g{g},
    _b{b},
    _a{a}
  {}

  Color4(const Color4&) = default;
  Color4(Color4&&) = default;
  Color4& operator=(const Color4&) = default;
  Color4& operator=(Color4&&) = default;

  void setRed(uint8_t r){_r = r;}
  void setGreen(uint8_t g){_g = g;}
  void setBlue(uint8_t b){_b = b;}
  void setAlpha(uint8_t a){_a = a;}
  uint8_t getRed() const {return _r;}
  uint8_t getGreen() const {return _g;}
  uint8_t getBlue() const {return _b;}
  uint8_t getAlpha() const {return _a;}
  float getfRed() const {return std::clamp(_r / 255.f, f_lo, f_hi);}    // clamp to cut-off float math errors.
  float getfGreen() const {return std::clamp(_g / 255.f, f_lo, f_hi);}
  float getfBlue() const {return std::clamp(_b / 255.f, f_lo, f_hi);}
  float getfAlpha() const {return std::clamp(_a / 255.f, f_lo, f_hi);}

private:
  uint8_t _r;
  uint8_t _g;
  uint8_t _b;
  uint8_t _a;
};

namespace colors
{
constexpr Color4 white {255, 255, 255};
constexpr Color4 black {0, 0, 0};
constexpr Color4 red {255, 0, 0};
constexpr Color4 green {0, 255, 0};
constexpr Color4 blue {0, 0, 255};
constexpr Color4 cyan {0, 255, 255};
constexpr Color4 magenta {255, 0, 255};
constexpr Color4 yellow {255, 255, 0};

// greys - more grays: https://en.wikipedia.org/wiki/Shades_of_gray 

constexpr Color4 gainsboro {224, 224, 224};
//constexpr Color4 lightgray {0.844f, 0.844f, 0.844f};
//constexpr Color4 silver {0.768f, 0.768f, 0.768f};
//constexpr Color4 mediumgray {0.76f, 0.76f, 0.76f};
//constexpr Color4 spanishgray {0.608f, 0.608f, 0.608f};
//constexpr Color4 gray {0.512f, 0.512f, 0.512f};
//constexpr Color4 dimgray {0.42f, 0.42f, 0.42f};
//constexpr Color4 davysgray {0.34f, 0.34f, 0.34f};
constexpr Color4 jet {53, 53, 53};
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
  void drawPixelArray(int first, int count, void* pixels, int pixelSize);
  void show();
  Vector2i getWindowSize() const;
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

  Vector2i windowSize = getWindowSize();
  std::stringstream().swap(ss);
  ss << "{w:" << windowSize._x << ",h:" << windowSize._y << "}";
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

void Renderer::drawPixelArray(int first, int count, void* pixels, int pixelSize)
{
  glInterleavedArrays(GL_C4UB_V2F, 0, pixels);
  glPointSize(pixelSize);
  glDrawArrays(GL_POINTS, first, count);
}

void Renderer::show()
{
  SDL_GL_SwapWindow(_window);
}

Vector2i Renderer::getWindowSize() const
{
  int w, h;
  SDL_GL_GetDrawableSize(_window, &w, &h);
  return Vector2i{w, h};
}

std::unique_ptr<Renderer> renderer {nullptr};

// A sprite represents a color image that can be drawn on a virtual screen. Pixels on the sprite
// are positioned on a coordinate space mapped as shown below.
//
//         row
//          ^
//          |
//          |
//   origin o----> col
//
class Sprite
{
public:
  Sprite();
  Sprite(std::vector<Color4> pixels, int width, int height);
  ~Sprite() = default;
  Sprite(const Sprite&) = default;
  Sprite(Sprite&&) = default;
  Sprite& operator=(const Sprite&) = default;
  Sprite& operator=(Sprite&&) = default;
  void setPixel(int row, int col, const Color4& color);
  const std::vector<Color4>& getPixels() const {return _pixels;}
  int getWidth() const {return _width;}
  int getHeight() const {return _height;}
private:
  std::vector<Color4> _pixels;
  int _width;
  int _height;
};

Sprite::Sprite() :
  _pixels{},
  _width{0},
  _height{0}
{}

Sprite::Sprite(std::vector<Color4> pixels, int width, int height) : 
  _width{width},
  _height{height},
  _pixels{pixels}
{}

void Sprite::setPixel(int row, int col, const Color4& color)
{
  _pixels[col + (row * _width)] = color;
}

// A virtual screen with fixed resolution independent of display resolution and window size. The
// screen is positioned centrally in the window with the ratio of virtual pixel size to real
// pixel size being calculated to fit the window dimensions.
//
// Pixels on the screen are arranged on a coordinate system with the origin in the bottom left
// most corner, rows ascending north and columns ascending east as shown below.
//
//      row
//       ^
//       |
//       |
//   pos o----> col
//
// note: virtual pixel sizes are limited to integer mulitiples of real pixels, i.e. integers.
//
class Screen
{
public:
  Screen(Vector2i windowSize);
  ~Screen() = default;
  void clear(const Color4& color);
  void clear(iRect region, const Color4& color);
  void drawPixel(int row, int col, const Color4& color);
  void drawSprite(int x, int y, const Sprite& sprite);
  void rescalePixels(Vector2i windowSize);
  void render();
private:
  // 12 byte pixels designed to work with glInterleavedArrays format GL_C4UB_V2F.
  struct Pixel
  {
    Color4 _color;
    float _x;
    float _y;
  };
private:
  static constexpr int screenWidth = 160;
  static constexpr int screenHeight = 160;
  static constexpr int pixelCount = screenWidth * screenHeight;
private:
  Vector2i _position;
  std::array<Pixel, pixelCount> _pixels; // flattened 2D array accessed (col + (row * width))
  int _pixelSize;
};

Screen::Screen(Vector2i windowSize)
{
  rescalePixels(windowSize);
}

void Screen::clear(const Color4& color)
{
  for(auto& pixel : _pixels)
    pixel._color = color;
}

void Screen::clear(iRect region, const Color4& color)
{

} 

void Screen::drawPixel(int row, int col, const Color4& color)
{
  assert(0 <= row && row < screenHeight);
  assert(0 <= col && col < screenWidth);
  _pixels[col + (row * screenWidth)]._color = color;
}

void Screen::drawSprite(int x, int y, const Sprite& sprite)
{
  assert(x >= 0 && y >= 0);

  const std::vector<Color4>& spritePixels {sprite.getPixels()};
  int spriteWidth {sprite.getWidth()};
  int spriteHeight {sprite.getHeight()};
  
  int spritePixelNum {0};
  for(int spriteRow = 0; spriteRow < spriteHeight; ++spriteRow){
    // if next row is above the screen.
    if(y + spriteRow >= screenHeight)
      break;

    // index of 1st screen pixel in next row being drawn.
    int screenRowIndex {x + ((y + spriteRow) * screenWidth)};   

    for(int spriteCol = 0; spriteCol < spriteWidth; ++spriteCol){
      // if the right side of the sprite falls outside the screen.
      if(x + spriteCol >= screenWidth)
        break;

      _pixels[screenRowIndex + spriteCol]._color = spritePixels[spritePixelNum];  
      ++spritePixelNum;
    }
  }
}

void Screen::rescalePixels(Vector2i windowSize)
{
  int pixelWidth = windowSize._x / screenWidth; 
  int pixelHeight = windowSize._y / screenHeight;
  _pixelSize = std::min(pixelWidth, pixelHeight);
  if(_pixelSize == 0)
    _pixelSize = 1;
  int pixelCenterOffset = _pixelSize / 2;
  _position._x = std::clamp((windowSize._x - (_pixelSize * screenWidth)) / 2, 0, windowSize._x);
  _position._y = std::clamp((windowSize._y - (_pixelSize * screenHeight)) / 2, 0, windowSize._y);
  for(int col = 0; col < screenWidth; ++col){
    for(int row = 0; row < screenHeight; ++row){
      int index = col + (row * screenWidth);
      Pixel& pixel = _pixels[index];
      pixel._x = _position._x + (col * _pixelSize) + pixelCenterOffset;
      pixel._y = _position._y + (row * _pixelSize) + pixelCenterOffset;
    }
  }
}

void Screen::render()
{
  auto now0 = std::chrono::high_resolution_clock::now();
  sk::renderer->drawPixelArray(0, pixelCount, static_cast<void*>(_pixels.data()), _pixelSize);
  auto now1 = std::chrono::high_resolution_clock::now();
  std::cout << "Screen::render execution time (us): "
            << std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0).count()
            << std::endl;
}

std::unique_ptr<Screen> screen {nullptr};

//------------------------------------------------------------------------------------------------
//  SNAKE                                                                                         
//------------------------------------------------------------------------------------------------

class Snake
{
public:
  enum MoveDirection { NORTH, SOUTH, EAST, WEST };
  enum MoveAxis { HORIZONTAL = 0, VERTICAL = 1 };
  enum MoveMagnitude { NEGATIVE = -1, POSITIVE = 1 };

  enum SegmentType { 
    BODY_EW, BODY_WE, BODY_NS, BODY_SN, CORNER_EN, CORNER_NE, CORNER_WN, CORNER_NW, CORNER_ES, 
    CORNER_SE, CORNER_WS, CORNER_SW, HEAD_E, HEAD_W, HEAD_N, HEAD_S, TAIL_E, TAIL_W, TAIL_N, 
    TAIL_S 
  };

  struct Segment
  {
    Vector2i _position;         // [x:col, y:row] w.r.t block world.
    MoveDirection _direction;   // segment's local direction of movement (seperate to the snake).
    SegmentType _type;          // used to draw the snake correctly (to select sprites).
  };

public:
  Snake(Vector2i worldDimensions, Vector2i);
  ~Snake() = default;
  void update(float dt);
  void setMoveDirection(MoveDirection direction);

private:
  static constexpr float moveFrequency {2.f};             // unit: move (block) jumps per second.
  static constexpr float movePeriod {1 / moveFrequency};
  static constexpr int appetite {1};                      // nuggets eaten until full (until grow).

private:
  void doMove();
  void calculateSegmentMoveDirection(Segment& segment, Vector2i oldPosition, Vector2i newPosition);
  void recalculateSegmentTypes();

private:
  std::vector<Segment> _segments;
  Vector2i _worldDimensions;
  MoveAxis _moveAxis;
  MoveMagnitude _moveMagnitude;
  float _moveClock;
  int _nuggetsEaten;
};

void Snake::update(float dt)
{
  _moveClock += dt;
  if(_moveClock > movePeriod){
    _moveClock = 0;
    doMove();
  }
}

void Snake::doMove()
{
  // move head segment.
  Segment& head = _segments[0];
  Vector2i oldPosition = head._position;
  head._position._x += (_moveAxis == HORIZONTAL) ? _moveMagnitude : 0;
  head._position._y += (_moveAxis == VERTICAL) ? _moveMagnitude : 0;
  calculateSegmentMoveDirection(head, oldPosition, head._position); 

  // wrap head around world if exceed world bounds.
  if(head._position._x < 0)
    head._position._x = _worldDimensions._x - 1;
  else if(head._position._x >= _worldDimensions._x - 1)
    head._position._x = 0;
  else if(head._position._y < 0)
    head._position._y = _worldDimensions._y - 1;
  else if(head._position._y >= _worldDimensions._y - 1)
    head._position._y = 0;

  // shift body segments.
  for(int i = 1; i < _segments.size(); ++i){
    Segment& segment = _segments[i];
    Vector2i temp = segment._position;
    segment._position = oldPosition;
    calculateSegmentMoveDirection(segment, temp, segment._position); 
    oldPosition = temp;
  }

  // add a segment if snake is full of nuggets.
  if(_neggetsEaten == appetite){
    Segment newTail {};
    newTail._position = oldPosition;

    // uses the old tail's old position (oldPosition) and the old tail's new position.
    calculateSegmentMoveDirection(segment, oldPosition, _segments.back()._position); 

    _segments.push_back(newTail);
  }

  // ensures the drawing reflects the change in snake position.
  recalculateSegmentTypes();
}

void Snake::calculateSegmentMoveDirection(Segment& segment, Vector2i oldPosition, Vector2i newPosition)
{
  int deltaRow = newPosition._y - oldPosition._y;
  int deltaCol = newPosition._x - oldPosition._x;

  // Segments (except the head) shift into the position of their neighbour which is closest to 
  // the head, this pattern of movement will always satisfy the below conditions. Thus if these
  // conditions are not met then the snake is dancing something funky!
  assert(-1 >= deltaRow && deltaRow <= 1);
  assert(-1 >= deltaCol && deltaCol <= 1);
  assert(deltaRow == 0 || deltaCol == 0);
  assert(deltaRow != deltaCol);

  MoveDirection direction;
  if(deltaRow != 0)
    direction = (deltaRow == 1) ? NORTH : SOUTH;
  else
    direction = (deltaRow == 1) ? NORTH : SOUTH;

  segment._moveDirection = direction;
}

void Snake::recalculateSegmentTypes()
{
  // calculate head type.
  switch(_segments.front()._moveDirection)
  {
  case NORTH:
    _segments.front()._type = HEAD_N;
    break;
  case SOUTH:
    _segments.front()._type = HEAD_S;
    break;
  case EAST:
    _segments.front()._type = HEAD_E;
    break;
  case WEST:
    _segments.front()._type = HEAD_W;
    break;
  };

  // calculate tail type.
  switch(_segments.back()._moveDirection)
  {
  case NORTH:
    _segments.back()._type = TAIL_N;
    break;
  case SOUTH:
    _segments.back()._type = TAIL_S;
    break;
  case EAST:
    _segments.back()._type = TAIL_E;
    break;
  case WEST:
    _segments.back()._type = TAIL_W;
    break;
  };

  // calculate types of all body segments.
  Segment dummy;
  for(int i = 1; i < _segments.size() - 1; ++i){
    Segment& thisSegment = _segments[i];

    Vector2i thisPosition = thisSegment._position;
    Vector2i headNeighbourPosition = _segments[i - 1]._position;
    Vector2i tailNeighbourPosition = _segments[i + 1]._position;

    MoveDirection directionToHead, directionToTail;
    calculateSegmentMoveDirection(dummy, thisPosition, headNeighbourPosition);
    directionToHead = dummy._moveDirection;
    calculateSegmentMoveDirection(dummy, thisPosition, tailNeighbourPosition);
    directionToTail = dummy._moveDirection;

    switch(directionToHead)
    {
    case NORTH:
      switch(directionToTail)
      {
        case NORTH:
          assert(0);
          break;
        case SOUTH:
          thisSegment._type = BODY_SN;
          break;
        case EAST:
          thisSegment._type = CORNER_EN;
          break;
        case WEST:
          thisSegment._type = CORNER_WN;
          break;
      }
      break;
    case SOUTH:
      switch(directionToTail)
      {
        case NORTH:
          thisSegment._type = BODY_NS;
          break;
        case SOUTH:
          assert(0);
          break;
        case EAST:
          thisSegment._type = CORNER_ES;
          break;
        case WEST:
          thisSegment._type = CORNER_WS;
          break;
      }
      break;
    case EAST:
      switch(directionToTail)
      {
        case NORTH:
          thisSegment._type = CORNER_NE;
          break;
        case SOUTH:
          thisSegment._type = CORNER_SE;
          break;
        case EAST:
          assert(0);
          break;
        case WEST:
          thisSegment._type = BODY_WE;
          break;
      }
      break;
    case WEST:
      switch(directionToTail)
      {
        case NORTH:
          thisSegment._type = CORNER_NW;
          break;
        case SOUTH:
          thisSegment._type = CORNER_SW;
          break;
        case EAST:
          thisSegment._type = BODY_EW;
          break;
        case WEST:
          assert(0);
          break;
      }
      break;
    }
  }
}

void Snake::setMoveDirection(MoveDirection direction)
{
  switch(direction)
  {
  case SOUTH:
    _moveAxis = VERTICAL;
    _moveMagnitude = NEGATIVE;
    break;
  case north:
    _moveAxis = VERTICAL;
    _moveMagnitude = POSITIVE;
    break;
  case east:
    _moveAxis = HORIZONTAL;
    _moveMagnitude = POSITIVE;
    break;
  case west:
    _moveAxis = HORIZONTAL;
    _moveMagnitude = NEGATIVE;
    break;
  }
}

class Game
{
public:
  enum ColorID { 
    COLOR_WORLD_BACKGROUND,
    COLOR_SNAKE_BODY_LIGHT,
    COLOR_SNAKE_BODY_SHADED,
    COLOR_SNAKE_BODY_SHADOW,
    COLOR_SNAKE_EYES,
    COLOR_SNAKE_TONGUE,
    COLOR_SNAKE_SPOTS
  };
public:
  Game();
  ~Game() = default;
  void draw();
private:
  static constexpr Vector2i worldDimensions {50, 50}; // [x:width(num cols), y:height(num rows)]
private:
  void generateSprites();
private:
  std::vector<Color4> _palette;

  // Sprite assets.
  std::vector<Sprite> _snakeSprites;
};

Snake::Snake()
{
  _palette.push_back(colors::jet);
  _palette.push_back(Color4(255, 217,  0));
  _palette.push_back(Color4(172, 146,  0));
  _palette.push_back(Color4( 42,  42, 42));
  _palette.push_back(Color4(214,   0,  0));
  _palette.push_back(Color4(214,   0,  0));
  _palette.push_back(Color4(  4,  69,  0));

  generateSprites();
}

void Snake::generateSprites()
{
  const std::vector<Color4>& p = _palette;

  _snakeSprites.push_back({{p[3], p[3], p[3], p[3], p[2], p[2], p[2], p[2], p[1], p[1], p[1], 
                            p[1], p[6], p[1], p[1], p[1]}, 4, 4});

}

void Snake::draw()
{
  sk::screen->clear(colors::gainsboro);
  sk::screen->drawSprite(30, 30, _snakeSprites[0]);
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
  static constexpr int windowWidth_px = 700;
  static constexpr int windowHeight_px = 200;
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
  _isDone{false},
  _game{}
{
}

App::~App()
{
}

void App::initialize()
{
  sk::log = std::make_unique<Log>();
  sk::input = std::make_unique<Input>();
  sk::screen = std::make_unique<Screen>(Vector2i{windowWidth_px, windowHeight_px});

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

  Vector2i windowSize = sk::renderer->getWindowSize();
  if(windowSize._x != windowWidth_px || windowSize._y != windowHeight_px)
    sk::screen->rescalePixels(windowSize);
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
      case SDL_WINDOWEVENT:
        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
          int w, h;
          w = event.window.data1;
          h = event.window.data2;
          sk::renderer->setViewport(iRect{0, 0, w, h});
          sk::screen->rescalePixels(Vector2i{w, h});
        }
        break;
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
  sk::screen->render();
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

