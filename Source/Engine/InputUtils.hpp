#pragma once

/* GLFW wrappers */

enum class Key
{
    /* The unknown key */
    eUnknown = -1,

    /* Printable keys */
    eSpace = 32,
    eApostrophe = 39,   /* ' */
    eComma = 44,        /* , */
    eMinus = 45,        /* - */
    ePeriod = 46,       /* . */
    eSlash = 47,        /* / */
    e0 = 48,
    e1 = 49,
    e2 = 50,
    e3 = 51,
    e4 = 52,
    e5 = 53,
    e6 = 54,
    e7 = 55,
    e8 = 56,
    e9 = 57,
    eSemicolon = 59,    /* ; */
    eEqual = 61,        /* = */
    eA = 65,
    eB = 66,
    eC = 67,
    eD = 68,
    eE = 69,
    eF = 70,
    eG = 71,
    eH = 72,
    eI = 73,
    eJ = 74,
    eK = 75,
    eL = 76,
    eM = 77,
    eN = 78,
    eO = 79,
    eP = 80,
    eQ = 81,
    eR = 82,
    eS = 83,
    eT = 84,
    eU = 85,
    eV = 86,
    eW = 87,
    eX = 88,
    eY = 89,
    eZ = 90,
    eLeftBracket = 91,  /* [ */
    eBackslash = 92,    /* \ */
    eRightBracket = 93, /* ] */
    eGraveAccent = 96,  /* ` */
    eWorld1 = 161,      /* non-US #1 */
    eWorld2 = 162,      /* non-US #2 */

    /* Function keys */
    eEscape = 256,
    eEnter = 257,
    eTab = 258,
    eBackspace = 259,
    eInsert = 260,
    eDelete = 261,
    eRight = 262,
    eLeft = 263,
    eDown = 264,
    eUp = 265,
    ePageUp = 266,
    ePageDown = 267,
    eHome = 268,
    eEnd = 269,
    eCapsLock = 280,
    eScrollLock = 281,
    eNumLock = 282,
    ePrintScreen = 283,
    ePause = 284,
    eF1 = 290,
    eF2 = 291,
    eF3 = 292,
    eF4 = 293,
    eF5 = 294,
    eF6 = 295,
    eF7 = 296,
    eF8 = 297,
    eF9 = 298,
    eF10 = 299,
    eF11 = 300,
    eF12 = 301,
    eF13 = 302,
    eF14 = 303,
    eF15 = 304,
    eF16 = 305,
    eF17 = 306,
    eF18 = 307,
    eF19 = 308,
    eF20 = 309,
    eF21 = 310,
    eF22 = 311,
    eF23 = 312,
    eF24 = 313,
    eF25 = 314,
    eNumPad0 = 320,
    eNumPad1 = 321,
    eNumPad2 = 322,
    eNumPad3 = 323,
    eNumPad4 = 324,
    eNumPad5 = 325,
    eNumPad6 = 326,
    eNumPad7 = 327,
    eNumPad8 = 328,
    eNumPad9 = 329,
    eNumPadDecimal = 330,
    eNumPadDivide = 331,
    eNumPadMultiply = 332,
    eNumPadSubtract = 333,
    eNumPadAdd = 334,
    eNumPadEnter = 335,
    eNumPadEqual = 336,
    eLeftShift = 340,
    eLeftControl = 341,
    eLeftAlt = 342,
    eLeftSuper = 343,
    eRightShift = 344,
    eRightControl = 345,
    eRightAlt = 346,
    eRightSuper = 347,
    eMenu = 348
};

enum class KeyAction
{
    eRelease,
    ePress,
    eRepeat
};

enum class MouseButton
{
    eLeft = 0,
    eRight,
    eMiddle,
    e4,
    e5,
    e6,
    e7,
    e8
};

enum class MouseButtonAction
{
    eRelease,
    ePress,
};

// TODO: another data structure to handle masks
enum class KeyMods : uint8_t
{
    eNone   = 0,
    eShift  = 1 << 0,
    eCtrl   = 1 << 1,
    eAlt    = 1 << 2,
    eSuper  = 1 << 3
};

constexpr KeyMods operator|(KeyMods lhs, KeyMods rhs)
{
    return static_cast<KeyMods>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr KeyMods operator&(KeyMods lhs, KeyMods rhs)
{
    return static_cast<KeyMods>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

constexpr bool HasMod(KeyMods mods, KeyMods flag)
{
    return (mods & flag) != KeyMods::eNone;
}

constexpr KeyMods ctrlKeyMod = platformMac ? KeyMods::eSuper : KeyMods::eCtrl;